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

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/smart_ptr/make_shared.hpp>

void PluginManager::loadPlugins(const std::string path_to_plugins)
{
  boost::filesystem::path path(path_to_plugins);
  if(!boost::filesystem::exists(path) ||
     !boost::filesystem::is_directory(path)) {
    return;
  }

  for(boost::filesystem::directory_iterator it(path);
      it != boost::filesystem::directory_iterator();
      ++it) {
    if(boost::filesystem::is_regular_file(it->path())) {
      // Create plugin and map it with the correspondent schema
      boost::shared_ptr<Plugin> plugin
                            = PluginFactory::createPlugin(it->path().string());
      _plugins.emplace(std::piecewise_construct,
                       std::forward_as_tuple(plugin->getSchema()),
                       std::forward_as_tuple(plugin));
    }
  }
}

void PluginManager::forwardUriToPlugin(const std::string uri) const
{
  std::string schema = uri.substr(0, uri.find("://"));

  try {
    boost::shared_ptr<Plugin> plugin = _plugins.at(schema);
    plugin->processUri(uri);
  } catch(std::out_of_range& e) {
    std::cerr << "Error: schema not support!" << std::endl << std::flush;
  }
}

