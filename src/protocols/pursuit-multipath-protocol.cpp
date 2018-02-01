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
          unsigned char path_id = req.getPathId();
          char* rfid = req.getReverseFid();

          PcrEntry pcr_entry(chunkuri, path_id, NULL, rfid);
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

          receivedMessage(in);

        } else if(type == CHUNK_RESPONSE) {
          FIFU_LOG_INFO("(PURSUIT Protocol) Received ChunkResponse for " + chararray_to_hex(ev.id));

          std::string uri = SCHEMA;
          uri.append(":").append(chararray_to_hex(ev.id));
          int chunk_no = strtoull(uri.substr(uri.size() - PURSUIT_ID_LEN_HEX_FORMAT,
                                  PURSUIT_ID_LEN_HEX_FORMAT).c_str(), NULL, 16);
          uri.erase(uri.size() - PURSUIT_ID_LEN_HEX_FORMAT);

          auto pcr_it = pending_requests.find(uri);
          if(pcr_it == pending_requests.end()) {
            break;
          }

          ChunkResponse resp((char*) ev.data, ev.data_len);
          pcr_it->second._payload += std::string(reinterpret_cast<const char*>(resp.getPayload()),
                                                 resp.getPayloadLen());

          if(resp.getPayloadLen() < CHUNK_SIZE) {
            MetaMessage* in = new MetaMessage();
            in->setUri(uri);
            in->setMessageType(MESSAGE_TYPE_RESPONSE);
            in->setContent("application/octet-stream", pcr_it->second._payload);

            FIFU_LOG_INFO("(PURSUIT Protocol) Received all ChunkResponse. Sending payload to core " + chararray_to_hex(ev.id));
            receivedMessage(in);

            // UNSUBSCRIBE
            pending_requests.erase(pcr_it);
          } else {
            ChunkRequest req((const char*) pcr_it->second.getReverseFid(), (char) 0);
            char reqBytes[req.size()];
            req.toBytes(reqBytes);

            std::stringstream chunk_no_ss;
            chunk_no_ss << std::setfill('0') << std::setw(16) << std::hex << ++chunk_no;

            publish_data(uri.append(chunk_no_ss.str()),
                         IMPLICIT_RENDEZVOUS,
                         (unsigned char*) pcr_it->second.getFid(),
                         (void*) reqBytes,
                         req.size());
          }
        }
      } break;

      case START_PUBLISH: {
        FIFU_LOG_INFO("(PURSUIT Protocol) START_PUBLISHER " + chararray_to_hex(ev.id));
        std::string uri = SCHEMA;
        uri.append(":").append(chararray_to_hex(ev.id));
        uri.erase(uri.size() - PURSUIT_ID_LEN_HEX_FORMAT);

        auto pcr_it = pending_requests.find(uri);
        if(pcr_it == pending_requests.end()) {
          break;
        }

        std::string fid = ev.FIDs.substr(0, FID_LEN * 8);
        std::string rfid = ev.FIDs.substr(FID_LEN * 8, FID_LEN * 8);

        // Convert FID and Reverse FID
        // from bit-array to byte-array (little-endian format)
        unsigned char f_bytearray[FID_LEN];
        unsigned char rf_bytearray[FID_LEN];
        for(int i = FID_LEN - 1; i >= 0; --i) {
          int rf_byte = 0;
          int f_byte = 0;
          for(int j = (FID_LEN - i - 1) * 8; j < ((FID_LEN - i) * 8); ++j) {
            rf_byte = (rf_byte << 1) | (rfid.at(j) == '1' ? 1 : 0);
            f_byte = (f_byte << 1) | (fid.at(j) == '1' ? 1 : 0);
          }

          rf_bytearray[i] = rf_byte;
          f_bytearray[i] = f_byte;
        }

        pcr_it->second.setPathId((char) 0);
        pcr_it->second.setFid((char*) f_bytearray);
        pcr_it->second.setReverseFid((char*) rf_bytearray);

        ChunkRequest req((const char*) rf_bytearray, (char) pcr_it->second.getPathId());
        char reqBytes[req.size()];
        req.toBytes(reqBytes);

        // Convert chunk number to request into hex format
        std::stringstream chunk_no_hex;
        chunk_no_hex << std::setfill('0') << std::setw(16) << std::hex << 0;

        publish_data(uri.append(chunk_no_hex.str()),
                     IMPLICIT_RENDEZVOUS,
                     (unsigned char*) pcr_it->second.getFid(),
                     (void*) reqBytes,
                     req.size());
        FIFU_LOG_INFO("(PURSUIT Protocol) Sent ChunkRequest to " + chararray_to_hex(ev.id));
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
    // Subscribe URI using multipath approach
    subscribeScope(msg->getUriString().erase(0, strlen(SCHEMA) + 1), IMPLICIT_RENDEZVOUS);
    subscribeUri(msg->getUriString().append("ffffffffffffffff"), MULTIPATH);

    PcrEntry pcr("", 0, NULL, NULL);
    pending_requests.emplace(msg->getUriString(), pcr);
  } else if(msg->getMessageType() == MESSAGE_TYPE_RESPONSE) {
    // Start publishing data

    auto pcr_it = pending_chunk_requests.find(msg->getUriString());
    if(pcr_it != pending_chunk_requests.end()) {
      for(auto const& pcr_entry : pcr_it->second) {
        std::string content_to_send;
        size_t requested_chunk = pcr_entry.getChunkNumber();
        bool send_all_chunks = false;

        do {
          // If this is true, then send all chunks without
          // being explicitly requested
          if(requested_chunk == 0xffffffffffffffff) {
            requested_chunk = 0;
            send_all_chunks = true;
          }

          content_to_send = msg->getContentData().substr(requested_chunk * CHUNK_SIZE, CHUNK_SIZE);

          ChunkResponse resp(content_to_send.c_str(),
                             content_to_send.size(),
                             pcr_entry.getPathId(),
                             1);

          char* respBytes;
          respBytes = new char[resp.size()];
          resp.toBytes(respBytes);

          publish_data(pcr_entry.getChunkUri(),
                       IMPLICIT_RENDEZVOUS,
                       (unsigned char*) pcr_entry.getReverseFid(),
                       (void*) respBytes,
                       resp.size());

          if(send_all_chunks) {
            requested_chunk++;
          }
        } while(content_to_send.size() == CHUNK_SIZE && send_all_chunks);
      }

      pending_chunk_requests.erase(pcr_it);
    }
  }

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

int PursuitMultipathProtocol::subscribeUri(const Uri uri, unsigned char strategy)
{
  std::string uri_wo_schema = uri.toUriEncodedString().erase(0, strlen(SCHEMA) + 1);

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Subscribing data related with " + uri.toString());
  ba->subscribe_info(hex_to_chararray(id),
                     hex_to_chararray(prefix_id),
                     strategy,
                     NULL,
                     0);

  return 0;
}

int PursuitMultipathProtocol::unsubscribeUri(const Uri uri, unsigned char strategy)
{
  std::string uri_wo_schema = uri.toUriEncodedString().erase(0, strlen(SCHEMA) + 1);

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Unsubscribing data related with " + uri.toString());
  ba->unsubscribe_info(hex_to_chararray(id),
                       hex_to_chararray(prefix_id),
                       strategy,
                       NULL,
                       0);

  return 0;
}

