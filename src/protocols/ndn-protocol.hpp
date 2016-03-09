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

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#define SCHEMA "ndn://"

using namespace ndn;

class NdnProtocol : public PluginProtocol
{
private:
  std::thread _msg_receiver;
  std::thread _msg_sender;
  std::thread _listen;
  Face _face;
  KeyChain _key_chain;

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
  void sendData(const std::string data_name, const std::string content);
};

#endif /* NDN_PROTOCOL__HPP_ */
