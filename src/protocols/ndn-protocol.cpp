/** Brief: NDN protocol plugin
 *  Copyright (C) 2016
 *  Jose Quevedo <quevedo@av.it.pt>
 *  Carlos Guimaraes <cguimaraes@av.it.pt>
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

#include "ndn-protocol.hpp"
#include "logger.hpp"

extern "C" NdnProtocol* create_plugin_object(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                                             ThreadPool& tp)
{
  return new NdnProtocol(queue, tp);
}

extern "C" void destroy_object(NdnProtocol* object)
{
  delete object;
}

///////////////////////////////////////////////////////////////////////////////
std::string removeSchemaFromUri(std::string uri)
{
  std::string schema_division = "://";
  size_t pos = uri.find(schema_division); //TODO: Schema division not found
  return uri.substr(pos+schema_division.length());
}

std::string createForeignUri(std::string o_uri)
{
  return SCHEMA + removeSchemaFromUri(o_uri);
}
///////////////////////////////////////////////////////////////////////////////

NdnProtocol::NdnProtocol(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                         ThreadPool& tp)
    : PluginProtocol(queue, tp)
{
}

NdnProtocol::~NdnProtocol()
{
}

void NdnProtocol::start()
{
  isRunning = true;

  _msg_receiver = std::thread(&NdnProtocol::startReceiver, this);
  _msg_sender = std::thread(&NdnProtocol::startSender, this);

  _listen = std::thread(&Face::processEvents, &_face, time::milliseconds::zero(), true);
}

void NdnProtocol::stop()
{
  isRunning = false;

  _face.shutdown();
  _listen.join();

  _msg_to_send.stop();

  _msg_receiver.detach();
  _msg_sender.join();
}

std::string NdnProtocol::installMapping(const std::string uri)
{
  std::string f_uri = createForeignUri(uri);

  _face.setInterestFilter(removeSchemaFromUri(f_uri),
    bind(&NdnProtocol::onInterest, this, _1, _2),
    RegisterPrefixSuccessCallback(),
    bind(&NdnProtocol::onRegisterFailed, this, _1, _2));

  return f_uri;
}

void NdnProtocol::onInterest(const InterestFilter& filter, const Interest& interest)
{
  MetaMessage* in = new MetaMessage();
  in->setUri(SCHEMA + interest.getName().toUri().substr(1, std::string::npos));

  FIFU_LOG_INFO("(NDN Protocol) Received Interest message to " + in->getUri());
  receivedMessage(in);
}

void NdnProtocol::sendData(const std::string data_name, const std::string content)
{
  // Create Data packet
  shared_ptr<Data> data = make_shared<Data>();
  data->setName(data_name);
  data->setFreshnessPeriod(time::seconds(10));
  data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

  // Sign Data packet with default identity
  _key_chain.sign(*data);

  // Return Data packet to the requester
  FIFU_LOG_INFO("(NDN Protocol) Sending Data message to " + data_name);
  _face.put(*data);
}

void NdnProtocol::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  FIFU_LOG_ERROR("(NDN Protocol) ERROR (" + reason + "): Failed to register prefix in local hub's daemon.")
  _face.shutdown();
}

void NdnProtocol::startReceiver()
{
}

void NdnProtocol::startSender()
{
  const MetaMessage* out;

  while(isRunning) {
    try {
      out = _msg_to_send.pop();
    } catch(...) {
      return;
    }

    // Schedule message processing
    FIFU_LOG_INFO("(NDN Protocol) Scheduling next message (" + out->getUri() + ") processing");
    std::function<void()> func(std::bind(&NdnProtocol::processMessage, this, out));
    _tp.schedule(std::move(func));
  }
}

void NdnProtocol::processMessage(const MetaMessage* msg)
{
  FIFU_LOG_INFO("(NDN Protocol) Processing message (" + msg->getUri() + ")");
  sendData(msg->getUri().substr(std::string(SCHEMA).size(),
                                std::string::npos),
           msg->getContentData());
}
