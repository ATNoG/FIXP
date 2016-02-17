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

extern "C" PursuitProtocol* create_plugin_object(boost::lockfree::queue<MetaMessage*>& queue)
{
  return new PursuitProtocol(queue);
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

PursuitProtocol::PursuitProtocol(boost::lockfree::queue<MetaMessage*>& queue)
    : PluginProtocol(queue)
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

  _msg_receiver.detach();
  _msg_sender.join();
}

std::string PursuitProtocol::installMapping(std::string uri)
{
  std::string f_uri = createForeignUri(uri);

  // Remove schema from uri
  if(f_uri.find(SCHEMA) == std::string::npos) {
    std::cerr << "Error: Invalid schema (" << f_uri << ")" << std::endl << std::flush;
    return NULL;
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
        in->_uri = SCHEMA + chararray_to_hex(ev.id);

        receivedMessage(in);
      } break;
    }
  }
}

void PursuitProtocol::startSender()
{
  MetaMessage* msg;

  while(isRunning) {
    while(_msg_to_send.pop(msg)) {
      //FIXME: Launch thread to handle send operation
      std::cout << "START_PUBLISH: " << msg->_uri << endl;
      ba->publish_data(hex_to_chararray(msg->_uri.substr(std::string(SCHEMA).size(),
                                                         std::string::npos)),
                                        DOMAIN_LOCAL,
                                        NULL,
                                        0,
                                        (void*) msg->_contentPayload.c_str(),
                                        msg->_contentPayload.size());

      // Release the kraken
      delete msg;
    }
  }
}

int PursuitProtocol::publishScope(std::string name)
{
  size_t id_init_pos    = name.length() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = name.substr(0, id_init_pos);
  std::string id        = name.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

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

  ba->publish_info(hex_to_chararray(id),
                   hex_to_chararray(prefix_id),
                   DOMAIN_LOCAL,
                   NULL,
                   0);

  return 0;
}

