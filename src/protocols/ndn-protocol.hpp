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

#ifndef NDN_PROTOCOL__HPP_
#define NDN_PROTOCOL__HPP_

#include "../plugin-protocol.hpp"
#include "concurrent-blocking-queue.hpp"
#include "thread-pool.hpp"

#include <thread>
#include <map>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#define SCHEMA "ndn://"
#define DEFAULT_PREFIX "fifu"

using namespace ndn;

static const uint32_t MAX_CHUNK_SIZE = ndn::MAX_NDN_PACKET_SIZE >> 1;
// TODO: implement concurrent requests -> static const int MAX_CONCURRENT_INTERESTS = 3;
// TODO: implement max retries -> static const int MAX_INTEREST_RETRIES = 3;

typedef std::map <std::string, std::string> ChunkContainer;

class NdnProtocol : public PluginProtocol
{
private:
  boost::asio::io_service _io_service;
  std::thread _msg_receiver;
  std::thread _msg_sender;
  std::thread _listen;
  Face _face;
  KeyChain _key_chain;
  Scheduler _scheduler;
  ChunkContainer _chunk_container;

public:
  NdnProtocol(ConcurrentBlockingQueue<const MetaMessage*>& queue,
              ThreadPool& tp);
  ~NdnProtocol();

  void start();
  void stop();

  std::string getProtocol() const { return "ndn"; };
  std::string installMapping(const std::string uri);

protected:
  void processMessage(const MetaMessage* msg);

private:
  void startReceiver();
  void startSender();

  void onInterest(const InterestFilter& filter, const Interest& interest);
  void onRegisterFailed(const Name& prefix, const std::string& reason);
  void onData(const Interest& interest, const Data& data);
  void onTimeout(const Interest& interest);

  void requestChunk(const Name& interest_name);
  void onChunk(const Interest& interest, const Data& data);
  void onChunkTimeout(const Interest& interest);

  void sendInterest(const std::string interest_name);
  void sendData(const std::string data_name, const std::string content);
};

#endif /* NDN_PROTOCOL__HPP_ */
