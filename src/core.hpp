/** Brief: FIXP core
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

#ifndef CORE__HPP_
#define CORE__HPP_

#include "plugin-manager.hpp"
#include "concurrent-blocking-queue.hpp"
#include "thread-pool.hpp"

#include <atomic>
#include <map>
#include <shared_mutex>
#include <string>
#include <vector>

class Core
{
private:
  std::atomic<bool> isRunning;

  PluginManager pm;
  ThreadPool& _tp;

  std::map<std::string, std::string> _mappings;
  mutable std::shared_timed_mutex _mappings_mutex;

  std::map<std::string, std::vector<std::string>> _waiting_for_response;
  mutable std::shared_timed_mutex _waiting_for_response_mutex;

  ConcurrentBlockingQueue<MetaMessage*> _queue;

public:
  Core(ThreadPool& tp)
    : isRunning(false),
      _tp(tp)
  { }

  ~Core()
  { }

  void start();
  void stop();

  void loadProtocol(std::string path);
  void loadConverter(std::string path);
  std::vector<std::string> createMapping(std::string o_uri);

private:
  void processMessage(MetaMessage* msg);
};

#endif /* CORE__HPP_ */

