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

#include <map>
#include <string>
#include <boost/lockfree/queue.hpp>

class Core
{
private:
  bool isRunning = false;

  PluginManager pm;

  std::map<std::string, std::string> _mappings;
  boost::lockfree::queue<MetaMessage*> _queue;

public:
  Core()
    : _queue(0)
  { }

  ~Core()
  {
    stop();
  }

  void start();
  void stop();

  void loadProtocol(std::string path);
  void loadConverter(std::string path);
  void createMapping(std::string uri);
};

#endif /* CORE__HPP_ */

