/** Brief: Plugin Manager
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

#ifndef PLUGIN_MANAGER__HPP_
#define PLUGIN_MANAGER__HPP_

#include "plugin-protocol.hpp"
#include "plugin-converter.hpp"
#include "concurrent-blocking-queue.hpp"
#include "thread-pool.hpp"

#include <map>
#include <thread>
#include <vector>
#include <boost/shared_ptr.hpp>

class PluginManager
{
private:
  std::map<std::string, boost::shared_ptr<PluginProtocol> > _protocols;
  std::map<std::string, boost::shared_ptr<PluginConverter> > _converters;

public:
  PluginManager()
  { };

  ~PluginManager()
  {
    // Delete protocol plugins
    for(auto item : _protocols) {
      item.second.reset();
    }

    // Delete converter plugins
    for (auto item : _converters) {
      item.second.reset();
    }

    // Delete plugins
    _protocols.clear();
    _converters.clear();
  }

  void stop();
  void loadProtocol(std::string path,
                    ConcurrentBlockingQueue<MetaMessage*>& queue,
                    ThreadPool& tp);
  void loadConverter(std::string path);

  std::vector<std::string> installMapping(std::string uri);
  boost::shared_ptr<PluginProtocol> getProtocolPlugin(std::string protocol);
  boost::shared_ptr<PluginConverter> getConverterPlugin(std::string o_uri, std::string f_uri);
};

#endif /* PLUGIN_MANAGER__HPP_ */

