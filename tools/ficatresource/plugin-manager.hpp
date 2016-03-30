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

#include "plugin.hpp"

#include <map>
#include <memory>

class PluginManager
{
private:
  std::map<std::string, std::shared_ptr<Plugin> > _plugins;

public:
  PluginManager() { };

  void loadPlugins(const std::string path_to_plugins);
  void forwardUriToPlugin(const std::string uri) const;
};

#endif /* PLUGIN_MANAGER__HPP_ */

