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

#ifndef FP7_PURSUIT_PROTOCOL__HPP_
#define FP7_PURSUIT_PROTOCOL__HPP_

#include "../plugin-protocol.hpp"
#include "concurrent-blocking-queue.hpp"
#include "thread-pool.hpp"

#include <thread>
#include <blackadder.hpp>

#define SCHEMA "pursuit://"
#define PURSUIT_ID_LEN_HEX_FORMAT 2 * PURSUIT_ID_LEN
#define DEFAULT_SCOPE "0000000000000000"

class PursuitProtocol : public PluginProtocol
{
private:
  Blackadder *ba;
  std::thread _msg_receiver;
  std::thread _msg_sender;

public:
  PursuitProtocol(ConcurrentBlockingQueue<MetaMessage*>& queue,
                  ThreadPool& tp);
  ~PursuitProtocol();

  void start();
  void stop();

  std::string getProtocol() { return "pursuit"; };
  std::string installMapping(std::string uri);

protected:
  void processMessage(MetaMessage* msg);

private:
  void startReceiver();
  void startSender();

  int publishScope(std::string name);
  int publishInfo(std::string name);
};

#endif /* FP7_PURSUIT_PROTOCOL__HPP_ */

