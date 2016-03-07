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

#include <cryptopp/sha.h>
#include <iostream>

extern "C" PursuitProtocol* create_plugin_object(ConcurrentBlockingQueue<MetaMessage*>& queue,
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
  f_uri.append(SCHEMA).append(DEFAULT_SCOPE);

  f_uri += chararray_to_hex(std::string(reinterpret_cast<const char*>(hash),
                                   CryptoPP::SHA1::DIGESTSIZE))
              .substr(0, PURSUIT_ID_LEN_HEX_FORMAT);

  return f_uri;
}
///////////////////////////////////////////////////////////////////////////////

PursuitProtocol::PursuitProtocol(ConcurrentBlockingQueue<MetaMessage*>& queue,
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

std::string PursuitProtocol::installMapping(std::string uri)
{
  std::string f_uri = createForeignUri(uri);

  // Remove schema from uri
  if(f_uri.find(SCHEMA) == std::string::npos) {
    std::cout << "[PURSUIT Protocol Plugin]" << std::endl
              << " - Foreign URI (" << f_uri
              << ") with an invalid schema"
              << "(Original: " << uri << ")" << std::endl;
    return "";
  }

  // Publish resource on the behalf of the original publisher
  publishInfo(f_uri.substr(std::string(SCHEMA).size(), std::string::npos));

  return f_uri;
}

void PursuitProtocol::startReceiver()
{
  while(isRunning) {
    Event ev;
    ba->getEvent(ev);
    switch (ev.type) {
      case START_PUBLISH: {
        MetaMessage* in = new MetaMessage();
        in->setUri(SCHEMA + chararray_to_hex(ev.id));
        std::cout << "[PURSUIT Protocol Plugin]" << std::endl
                  << " - Received request for "
                  << in->getUri() << std::endl;

        std::cout << "[PURSUIT Protocol Plugin]" << std::endl
                  << " - Retrieving request of " << in->getUri()
                  << " to FIXP" << std::endl;
        receivedMessage(in);
      } break;
    }
  }
}

void PursuitProtocol::startSender()
{
  MetaMessage* out;

  while(isRunning) {
    try {
      out = _msg_to_send.pop();
    } catch(...) {
      return;
    }

    std::cout << "[PURSUIT Protocol Plugin]" << std::endl
              << "Processing next message in the queue ("
              << out->getUri() << ")" << std::endl;

    std::function<void()> func(std::bind(&PursuitProtocol::processMessage, this, out));
    _tp.schedule(std::move(func));
  }
}

void PursuitProtocol::processMessage(MetaMessage* msg)
{
  std::cout << "[PURSUIT Protocol Plugin]" << std::endl
            << " - Starting publishing item ("
            << msg->getUri() << ")" << std::endl;

  ba->publish_data(hex_to_chararray(msg->getUri().substr(std::string(SCHEMA).size(),
                                                     std::string::npos)),
                                    DOMAIN_LOCAL,
                                    NULL,
                                    0,
                                    (void*) msg->getContentData().c_str(),
                                    msg->getContentData().size());

  // Release the kraken
  delete msg;
}

int PursuitProtocol::publishScope(std::string name)
{
  size_t id_init_pos    = name.length() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = name.substr(0, id_init_pos);
  std::string id        = name.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  std::cout << "[PURSUIT Protocol Plugin]" << std::endl
            << " - Publishing scope " << name << std::endl;
  ba->publish_scope(hex_to_chararray(id),
                    hex_to_chararray(prefix_id),
                    DOMAIN_LOCAL,
                    NULL,
                    0);

  return 0;
}

int PursuitProtocol::publishInfo(std::string name)
{
  size_t id_init_pos    = name.length() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = name.substr(0, id_init_pos);
  std::string id        = name.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  std::cout << "[PURSUIT Protocol Plugin]" << std::endl
            << " - Publishing item " << name << std::endl;
  ba->publish_info(hex_to_chararray(id),
                   hex_to_chararray(prefix_id),
                   DOMAIN_LOCAL,
                   NULL,
                   0);

  return 0;
}

