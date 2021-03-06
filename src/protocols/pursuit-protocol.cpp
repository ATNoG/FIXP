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

#include "pursuit-protocol.hpp"
#include "logger.hpp"

#include <cryptopp/sha.h>
#include <iostream>

extern "C" PursuitProtocol* create_plugin_object(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                                                 ThreadPool& tp)
{
  return new PursuitProtocol(queue, tp);
}

extern "C" void destroy_object(PursuitProtocol* object)
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

PursuitProtocol::PursuitProtocol(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                                 ThreadPool& tp)
    : PluginProtocol(queue, tp)
{
  // Blackadder running in user space
  ba = Blackadder::Instance(true);
  publishScope(DEFAULT_SCOPE);
}

PursuitProtocol::~PursuitProtocol()
{
  ba->disconnect();
  delete ba;
}

void PursuitProtocol::start()
{
  isRunning = true;

  _msg_receiver = std::thread(&PursuitProtocol::startReceiver, this);
  _msg_sender = std::thread(&PursuitProtocol::startSender, this);
}

void PursuitProtocol::stop()
{
  isRunning = false;
  _msg_to_send.stop();

  _msg_receiver.detach();
  _msg_sender.join();
}

std::string PursuitProtocol::installMapping(const std::string uri)
{
  Uri f_uri(createForeignUri(uri));
  std::string uri_wo_schema = f_uri.toUriEncodedString().erase(0, strlen(SCHEMA) + 1);

  // Publish resource on the behalf of the original publisher
  publishInfo(uri_wo_schema);

  return f_uri.toString();
}

void PursuitProtocol::startReceiver()
{
  while(isRunning) {
    Event ev;
    ba->getEvent(ev);
    switch (ev.type) {
      case START_PUBLISH: {
        MetaMessage* in = new MetaMessage();
        in->setUri(std::string(SCHEMA) + ":" + chararray_to_hex(ev.id));
        in->setMessageType(MESSAGE_TYPE_REQUEST);

        FIFU_LOG_INFO("(PURSUIT Protocol) Received START_PUBLISH to " + in->getUriString());
        receivedMessage(in);
      } break;

      case PUBLISHED_DATA: {
        MetaMessage* in = new MetaMessage();
        in->setUri(std::string(SCHEMA) + ":" + chararray_to_hex(ev.id));
        in->setMessageType(MESSAGE_TYPE_RESPONSE);
        in->setContent("", std::string(reinterpret_cast<const char*>(ev.data),
                                                                     ev.data_len));
        in->setKeepAlive(true);

        FIFU_LOG_INFO("(PURSUIT Protocol) Received PUBLISH_DATA to " + in->getUriString());
        receivedMessage(in);

      } break;
    }
  }
}

void PursuitProtocol::startSender()
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
    std::function<void()> func(std::bind(&PursuitProtocol::processMessage, this, out));
    _tp.schedule(std::move(func));
  }
}

void PursuitProtocol::processMessage(const MetaMessage* msg)
{
  FIFU_LOG_INFO("(PURSUIT Protocol) Processing message (" + msg->getUriString() + ")");

  if(msg->getMessageType() == MESSAGE_TYPE_REQUEST) {
    // Subscribe URI
    subscribeUri(msg->getUri());

  } else if(msg->getMessageType() == MESSAGE_TYPE_RESPONSE) {
    // Start publishing data
    publishUriContent(msg->getUri(), (void*) msg->getContentData().c_str(), msg->getContentData().size());
  }

  delete msg;
}

int PursuitProtocol::publishScope(const std::string name)
{
  size_t id_init_pos    = name.length() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = name.substr(0, id_init_pos);
  std::string id        = name.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Publishing scope (" + name + ")");
  ba->publish_scope(hex_to_chararray(id),
                    hex_to_chararray(prefix_id),
                    DOMAIN_LOCAL,
                    NULL,
                    0);

  return 0;
}

int PursuitProtocol::publishInfo(const std::string name)
{
  size_t id_init_pos    = name.length() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = name.substr(0, id_init_pos);
  std::string id        = name.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  FIFU_LOG_INFO("(PURSUIT Protocol) Publishing item (" + name + ")");
  ba->publish_info(hex_to_chararray(id),
                   hex_to_chararray(prefix_id),
                   DOMAIN_LOCAL,
                   NULL,
                   0);

  return 0;
}

int PursuitProtocol::publishUriContent(const Uri uri, void* content, size_t content_size)
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

int PursuitProtocol::subscribeUri(const Uri uri)
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

int PursuitProtocol::unsubscribeUri(const Uri uri)
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

