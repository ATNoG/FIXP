/** Brief: FP7 PURSUIT (Blackadder) protocol plugin
 *  Copyright (C) 2016  Carlos Guimaraes <cguimaraes@av.it.pt>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "pursuit-multipath-protocol.hpp"
#include "logger.hpp"
#include "pursuit/chunk.hpp"

#include <cryptopp/sha.h>
#include <iostream>
#include <map>

extern "C" PursuitMultipathProtocol* create_plugin_object(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                                                 ThreadPool& tp)
{
  return new PursuitMultipathProtocol(queue, tp);
}

extern "C" void destroy_object(PursuitMultipathProtocol* object)
{
  delete object;
}

///////////////////////////////////////////////////////////////////////////////
std::string createForeignUri(std::string o_uri)
{
  byte hash[CryptoPP::SHA1::DIGESTSIZE];
  CryptoPP::SHA1().CalculateDigest(hash,
                                   reinterpret_cast<const byte*>(o_uri.c_str()),
                                   o_uri.size());
  std::string f_uri;
  f_uri.append(SCHEMA).append(":").append(DEFAULT_SCOPE);

  f_uri += chararray_to_hex(std::string(reinterpret_cast<const char*>(hash),
                                   CryptoPP::SHA1::DIGESTSIZE))
              .substr(0, PURSUIT_ID_LEN_HEX_FORMAT);

  return f_uri;
}
///////////////////////////////////////////////////////////////////////////////

PursuitMultipathProtocol::PursuitMultipathProtocol(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                                 ThreadPool& tp)
    : PluginProtocol(queue, tp)
{
  // Blackadder running in user space
  ba = Blackadder::Instance(true);
  publishScope(DEFAULT_SCOPE, DOMAIN_LOCAL);
}

PursuitMultipathProtocol::~PursuitMultipathProtocol()
{
  ba->disconnect();
  delete ba;
}

void PursuitMultipathProtocol::start()
{
  isRunning = true;

  _msg_receiver = std::thread(&PursuitMultipathProtocol::startReceiver, this);
  _msg_sender = std::thread(&PursuitMultipathProtocol::startSender, this);
}

void PursuitMultipathProtocol::stop()
{
  isRunning = false;
  _msg_to_send.stop();

  _msg_receiver.detach();
  _msg_sender.join();
}

std::string PursuitMultipathProtocol::installMapping(const std::string uri)
{
  Uri f_uri(createForeignUri(uri));
  std::string uri_wo_schema = f_uri.toUriEncodedString().erase(0, strlen(SCHEMA) + 1);

  // Publish resource on the behalf of the original publisher
  publishScope(uri_wo_schema, DOMAIN_LOCAL);
  subscribeScope(uri_wo_schema, IMPLICIT_RENDEZVOUS);
  publishInfo(uri_wo_schema + "ffffffffffffffff", MULTIPATH);

  return f_uri.toString();
}

void PursuitMultipathProtocol::startReceiver()
{
  while(isRunning) {
    Event ev;
    ba->getEvent(ev);
    switch (ev.type) {
      case PUBLISHED_DATA: {
        unsigned char type = ((char *)ev.data)[0];
        if(type == CHUNK_REQUEST) {
          FIFU_LOG_INFO("(PURSUIT Protocol) Received ChunkRequest for " + chararray_to_hex(ev.id));

          std::string chunkuri = SCHEMA;
          chunkuri.append(":").append(chararray_to_hex(ev.id));
          std::string uri = SCHEMA;
          uri.append(":").append(chararray_to_hex(ev.id));
          uri.erase(uri.size() - PURSUIT_ID_LEN_HEX_FORMAT);

          ChunkRequest req((char*) ev.data);
          unsigned char fid = req.getPathId();
          char* rfid = req.getReverseFid();

          PcrEntry pcr_entry(chunkuri, fid, rfid);
          auto pcr_it = pending_chunk_requests.find(uri);
          if(pcr_it != pending_chunk_requests.end()) {
            pcr_it->second.push_back(pcr_entry);
          } else {
            std::vector<PcrEntry> pcr_entry_vec;
            pcr_entry_vec.push_back(pcr_entry);
            pending_chunk_requests.emplace(uri, pcr_entry_vec);
          }

          MetaMessage* in = new MetaMessage();
          in->setUri(uri);
          in->setMessageType(MESSAGE_TYPE_REQUEST);

          FIFU_LOG_INFO("(PURSUIT Protocol) Received ChunkRequest to " + in->getUriString());
          receivedMessage(in);

        } else if(type == CHUNK_RESPONSE) {
        // TODO
        }
      } break;
    }
  }
}

void PursuitMultipathProtocol::startSender()
{
  const MetaMessage* out;

  while(isRunning) {
    try {
      out = _msg_to_send.pop();
    } catch(...) {
      return;
    }

    // Schedule message processing
    FIFU_LOG_INFO("(PURSUIT Protocol) Scheduling next message (" + out->getUriString() + ") processing");
    std::function<void()> func(std::bind(&PursuitMultipathProtocol::processMessage, this, out));
    _tp.schedule(std::move(func));
  }
}

void PursuitMultipathProtocol::processMessage(const MetaMessage* msg)
{
  FIFU_LOG_INFO("(PURSUIT Protocol) Processing message (" + msg->getUriString() + ")");

  if(msg->getMessageType() == MESSAGE_TYPE_REQUEST) {
    // Subscribe URI
    subscribeUri(msg->getUri());

  } else if(msg->getMessageType() == MESSAGE_TYPE_RESPONSE) {
    // Start publishing data

    auto pcr_it = pending_chunk_requests.find(msg->getUriString());
    if(pcr_it != pending_chunk_requests.end()) {
      for(auto const& pcr_entry : pcr_it->second) {
        ChunkResponse* resp = new ChunkResponse(msg->getContentData().c_str(), msg->getContentData().size(),
                                                pcr_entry.getFid(), 1);
        char* respBytes;
        respBytes = new char[resp->size()];
        resp->toBytes(respBytes);

        publish_data(pcr_entry.getChunkUri(),
                     IMPLICIT_RENDEZVOUS,
                     (unsigned char*) pcr_entry.getReverseFid(),
                     (void*) respBytes,
                     resp->size());
      }

      pending_chunk_requests.erase(pcr_it);
    }
  }

  // Release the kraken
  delete msg;
}

int PursuitMultipathProtocol::publishScope(const std::string name, unsigned char strategy)
{
  size_t id_init_pos    = name.length() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = name.substr(0, id_init_pos);
  std::string id        = name.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Publishing scope (" + name + ")");
  ba->publish_scope(hex_to_chararray(id),
                    hex_to_chararray(prefix_id),
                    strategy,
                    NULL,
                    0);

  return 0;
}

int PursuitMultipathProtocol::subscribeScope(const std::string name, unsigned char strategy)
{
  size_t id_init_pos    = name.length() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = name.substr(0, id_init_pos);
  std::string id        = name.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Subscribing scope (" + name + ")");
  ba->subscribe_scope(hex_to_chararray(id),
                      hex_to_chararray(prefix_id),
                      strategy,
                      NULL,
                      0);

  return 0;
}


int PursuitMultipathProtocol::publishInfo(const std::string name, unsigned char strategy)
{
  size_t id_init_pos    = name.length() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = name.substr(0, id_init_pos);
  std::string id        = name.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Publishing item (" + name + ")");
  ba->publish_info(hex_to_chararray(id),
                   hex_to_chararray(prefix_id),
                   strategy,
                   NULL,
                   0);

  return 0;
}

int PursuitMultipathProtocol::publish_data(const Uri uri, unsigned char strategy, unsigned char* fid, void* content, size_t content_size)
{
  std::string uri_wo_schema = uri.toString().erase(0, strlen(SCHEMA) + 1);

  ba->publish_data(hex_to_chararray(uri_wo_schema),
                   strategy,
                   fid,
                   (fid == NULL ? 0 : FID_LEN),
                   content,
                   content_size);

  return 0;
}

int PursuitMultipathProtocol::publishUriContent(const Uri uri, void* content, size_t content_size)
{
  std::string uri_wo_schema = uri.toUriEncodedString().erase(0, strlen(SCHEMA) + 1);

  FIFU_LOG_INFO("(PURSUIT Protocol) Publishing Data related with " + uri.toString());
  ba->publish_data(hex_to_chararray(uri_wo_schema),
                   DOMAIN_LOCAL,
                   NULL,
                   0,
                   content,
                   content_size);

  return 0;
}

int PursuitMultipathProtocol::subscribeUri(const Uri uri)
{
  std::string uri_wo_schema = uri.toUriEncodedString().erase(0, strlen(SCHEMA) + 1);

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Subscribing data related with " + uri.toString());
  ba->subscribe_info(hex_to_chararray(id),
                     hex_to_chararray(prefix_id),
                     DOMAIN_LOCAL,
                     NULL,
                     0);

  return 0;
}

int PursuitMultipathProtocol::unsubscribeUri(const Uri uri)
{
  std::string uri_wo_schema = uri.toUriEncodedString().erase(0, strlen(SCHEMA) + 1);

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Unsubscribing data related with " + uri.toString());
  ba->unsubscribe_info(hex_to_chararray(id),
                       hex_to_chararray(prefix_id),
                       DOMAIN_LOCAL,
                       NULL,
                       0);

  return 0;
}

