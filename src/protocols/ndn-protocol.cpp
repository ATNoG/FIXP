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
std::string createForeignUri(std::string o_uri)
{
  Uri uri(o_uri);

  std::string f_uri;
  f_uri.append(SCHEMA).append(":")
       .append("/").append(DEFAULT_PREFIX);

  if(uri.getAuthority() != "") {
    f_uri.append("/").append(uri.getAuthority());
  }

  f_uri.append(uri.getPath())
       .append(escapeString("?"))
       .append(uri.getUriEncodedQuery())
       .append(escapeString("#"))
       .append(uri.getUriEncodedFragment());

  return f_uri;
}

std::string cleanName(Name name)
{
    int i = 0;
    while (!(name[i].isVersion() ||
             name[i].isTimestamp() ||
             name[i].isSegment() ||
             name[i].isSequenceNumber() ||
             name[i].isSegmentOffset()))
    {
      i++;
    }
    return name.getPrefix(i).toUri();
}

///////////////////////////////////////////////////////////////////////////////

NdnProtocol::NdnProtocol(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                         ThreadPool& tp)
    : PluginProtocol(queue, tp)
    , _face(_io_service)
    , _scheduler(_io_service)
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

  _face.setInterestFilter(InterestFilter("/fifu"),
                          bind(&NdnProtocol::onInterest, this, _1, _2),
                          RegisterPrefixSuccessCallback(),
                          bind(&NdnProtocol::onRegisterFailed, this, _1, _2));
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
  Uri f_uri(createForeignUri(uri));
  std::string uri_wo_schema = f_uri.toUriEncodedString().erase(0, strlen(SCHEMA) + 1);

  return f_uri.toString();
}

void NdnProtocol::onInterest(const InterestFilter& filter, const Interest& interest)
{
  MetaMessage* in = new MetaMessage();

  // Remove trailing Version and/or Segment Number
  // TODO: Handle request for an specific chunk, handle request for specific version
  std::string uri;
  const Name interest_name(interest.getName());
  if (interest_name[-1].isSegment())
  {
    uri = interest_name.getPrefix (interest_name.size ()-2).toUri();
    return;
  }
  else if (interest_name[-1].isVersion())
    uri = interest_name.getPrefix (interest_name.size ()-1).toUri();
  else
    uri = interest_name.toUri();

  in->setUri(std::string(SCHEMA) + ":" + uri);
  in->setMessageType(MESSAGE_TYPE_REQUEST);

  FIFU_LOG_INFO("(NDN Protocol) Received Interest message to " + in->getUriString());
  receivedMessage(in);
}

void NdnProtocol::sendInterest(const std::string interest_name)
{
  Name name = Name(interest_name).appendVersion(0);
  Interest interest(name);
  interest.setInterestLifetime(time::milliseconds(5000));
  interest.setMustBeFresh(true);

  FIFU_LOG_INFO("(NDN Protocol) Sending Interest message to " + interest_name);
  _face.expressInterest(interest,
                        bind(&NdnProtocol::onData, this,  _1, _2),
                        bind(&NdnProtocol::onTimeout, this, _1));

  FIFU_LOG_INFO("(NDN Protocol) ProcessEvents finished...");
}

void NdnProtocol::sendData(const std::string data_name, const std::string content)
{
  FIFU_LOG_INFO("(NDN Protocol) Sending Data message to " + data_name);

  //Determine number of chunks
  uint32_t chunk_count = 1 + (content.size() - 1) / MAX_CHUNK_SIZE;

  if (chunk_count == 1)
  {
    // Create Data packet
    shared_ptr<Data> data = make_shared<Data>();
    data->setName(data_name);
    data->setFreshnessPeriod(time::seconds(100));
    data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

    // Sign Data packet with default identity
    _key_chain.sign(*data);

    // Return Data packet to the requester
    _face.put(*data);
  }
  else
  {
    for (int i = 0; i < chunk_count; i++)
    {
      //Get chunk from content
      uint32_t chunkLength = MAX_CHUNK_SIZE;
      if (i == chunk_count - 1)
        chunkLength = content.size() % MAX_CHUNK_SIZE;
      std::string chunk = content.substr(i*MAX_CHUNK_SIZE,chunkLength);

      //Prepare and send chunk of Data
      shared_ptr<Data> data = make_shared<Data> (
      //TODO: Define our own approach regarding versioning
      Name(data_name).appendVersion(0).appendSegment(i)
      );
      data->setFreshnessPeriod (time::seconds(100));
      data->setContent (reinterpret_cast<const uint8_t *>(chunk.c_str()),chunk.size());
      data->setFinalBlockId (name::Component::fromSegment (chunk_count-1));
      _key_chain.sign(*data);
      _face.put (*data);
      FIFU_LOG_INFO("Pushing Chunk " + data->getName().toUri());
    }
  }
}

void NdnProtocol::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  FIFU_LOG_ERROR("(NDN Protocol) ERROR (" + reason + "): Failed to register prefix in local hub's daemon.")
  _face.shutdown();
}

void NdnProtocol::onData(const Interest& interest, const Data& data)
{
  //Check whether it is a chunk or a complete data
  if (data.getName()[-1].isSegment())
    onChunk(interest,data);
  else
  {
    Block content = data.getContent();
    MetaMessage* in = new MetaMessage();
    in->setUri(std::string(SCHEMA) + ":" + cleanName(interest.getName()));
    in->setMessageType(MESSAGE_TYPE_RESPONSE);
    in->setContent("", std::string(reinterpret_cast<const char*>(content.value()),
                                                                 content.value_size()));

    FIFU_LOG_INFO("(NDN Protocol) Received Data message to " + in->getUriString());
    receivedMessage(in);
  }
}

void NdnProtocol::requestChunk(const Name& interest_name)
{
  Interest interest(interest_name);
  interest.setInterestLifetime(time::milliseconds(5000));
  interest.setMustBeFresh(true);

  FIFU_LOG_INFO("(NDN Protocol) Requesting Chunk " + interest_name.toUri());
  _face.expressInterest(interest,
                        bind(&NdnProtocol::onChunk, this,  _1, _2),
                        bind(&NdnProtocol::onChunkTimeout, this, _1));
}

void NdnProtocol::onChunk(const Interest& interest, const Data& data)
{
  const Name data_name = data.getName();
  uint64_t chunk_no = data_name[-1].toSegment();
  std::string content_name = cleanName(data_name);
  uint64_t last_chunk_no = data.getFinalBlockId().toSegment();
  std::cout << "Chunk " << chunk_no << "/" << last_chunk_no << std::endl;
  auto it = _chunk_container.find(content_name);
  if (chunk_no == 0)
  {
    _chunk_container[content_name] = std::string(reinterpret_cast<const char *>(data.getContent().value()),
                                                       data.getContent().value_size());
  }
  else if(it != _chunk_container.end())
  {
    it->second += std::string(reinterpret_cast<const char *>(data.getContent().value()),
                                                       data.getContent().value_size());
  }
  else
      FIFU_LOG_ERROR("Something went wrong with the chunk container for " + content_name);

  //TODO: For now lets assume FinalBlockId will be available in every chunk
  it = _chunk_container.find(content_name);
  if(chunk_no==last_chunk_no)
  {
    if(it != _chunk_container.end())
    {
      std::string content = it->second;
      MetaMessage* in = new MetaMessage();
      in->setUri(std::string(SCHEMA) + ":" + content_name);
      in->setMessageType(MESSAGE_TYPE_RESPONSE);
      in->setContent("", content);
      FIFU_LOG_INFO("(NDN Protocol) Received Data message to " + in->getUriString());
      receivedMessage(in);
    }
    else
      FIFU_LOG_ERROR("Something went wrong with the chunk container for " + content_name);
  }
  else
  {
    const Name next_chunk = data_name.getPrefix(data_name.size() - 1)
                                            .appendSegment(chunk_no + 1);
    _scheduler.scheduleEvent(time::milliseconds(0),
                            bind(&NdnProtocol::requestChunk, this, next_chunk));
  }
}

void NdnProtocol::onChunkTimeout(const Interest& interest)
{
  FIFU_LOG_INFO("(NDN Protocol) Chunk request timeout to " +
                std::string(SCHEMA) + ":" + interest.getName().toUri());
  //TODO: handle retries
}

void NdnProtocol::onTimeout(const Interest& interest)
{
  FIFU_LOG_INFO("(NDN Protocol) Interest timeout to " +
                std::string(SCHEMA) + ":" + interest.getName().toUri());
}

void NdnProtocol::startReceiver()
{
  _listen = std::thread(&Face::processEvents, &_face, time::milliseconds::zero(), true);
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
    FIFU_LOG_INFO("(NDN Protocol) Scheduling next message (" + out->getUriString() + ") processing");
    std::function<void()> func(std::bind(&NdnProtocol::processMessage, this, out));
    _tp.schedule(std::move(func));
  }
}

void NdnProtocol::processMessage(const MetaMessage* msg)
{
  FIFU_LOG_INFO("(NDN Protocol) Processing message (" + msg->getUriString() + ")");
  std::string uri_wo_schema = msg->getEncodedUriString().erase(0, strlen(SCHEMA) + 1);

  if(msg->getMessageType() == MESSAGE_TYPE_REQUEST) {
    // Send Interest message
    sendInterest(uri_wo_schema);

  } else if(msg->getMessageType() == MESSAGE_TYPE_RESPONSE) {
    // Send Data message
    sendData(uri_wo_schema, msg->getContentData());
  }
}
