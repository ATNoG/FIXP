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

#include "plugin-manager.hpp"
#include "plugin-factory.hpp"

#include <dirent.h>
#include <iostream>

void PluginManager::loadPlugins(const std::string path_to_plugins)
{
  DIR *dir = opendir(path_to_plugins.c_str());
  if(dir == NULL) {
    std::cout << "Error: Cannot open plugins folder "
              << std::endl << std::flush;
    return;
  }

  struct dirent* dirEntry;
  while(dirEntry = readdir(dir)) {
    if(dirEntry->d_type == DT_REG) {
      // Create plugin and map it with the correspondent schema
      std::shared_ptr<Plugin> plugin
                        = PluginFactory::createPlugin(path_to_plugins + "/" +
                                                      dirEntry->d_name);
      _plugins.emplace(std::piecewise_construct,
                       std::forward_as_tuple(plugin->getSchema()),
                       std::forward_as_tuple(plugin));
    }
  }
}

void PluginManager::forwardUriToPlugin(const Uri uri) const
{
  try {
    std::shared_ptr<Plugin> plugin = _plugins.at(uri.getSchema());
    plugin->processUri(uri.toString());
  } catch(std::out_of_range& e) {
    std::cerr << "Error: schema not support!" << std::endl << std::flush;
  }
}

