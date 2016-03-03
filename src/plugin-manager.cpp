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
#include "plugin-protocol-factory.hpp"
#include "plugin-converter-factory.hpp"

#include <iostream>
#include <thread>
#include <boost/filesystem.hpp>

///////////////////////////////////////////////////////////////////////////////
std::string extractSchema(std::string uri)
{
  return uri.substr(0, uri.find("://"));
}

///////////////////////////////////////////////////////////////////////////////
void PluginManager::stop()
{
  // Stop protocol plugins
  for(auto item : _protocols) {
    item.second->stop();
  }
}

void PluginManager::loadProtocol(std::string path,
                                 ConcurrentBlockingQueue<MetaMessage*>& queue,
                                 ThreadPool& tp)
{
  boost::filesystem::path file(path);
  if(!boost::filesystem::exists(file) ||
     !boost::filesystem::is_regular_file(file)) {
    return;
  }

  // Create protocol plugin
  boost::shared_ptr<PluginProtocol> protocol
         = PluginProtocolFactory::createPlugin(path, queue, tp);
  _protocols.emplace(std::piecewise_construct,
                     std::forward_as_tuple(protocol->getProtocol()),
                     std::forward_as_tuple(protocol));
  std::cout << "[FIXP (PM)]" << std::endl
            << " - Loading " << protocol->getProtocol()
            << " protocol...done!" << std::endl;

  protocol->start();
  std::cout << "[FIXP (PM)]" << std::endl
            << " - Starting " << protocol->getProtocol()
            << " protocol...done!" << std::endl;
}

void PluginManager::loadConverter(std::string path)
{
  boost::filesystem::path file(path);
  if(!boost::filesystem::exists(file) ||
     !boost::filesystem::is_regular_file(file)) {
    return;
  }

  // Create converter plugin
  boost::shared_ptr<PluginConverter> converter
         = PluginConverterFactory::createPlugin(path);
  _converters.emplace(std::piecewise_construct,
                      std::forward_as_tuple(converter->getProtocolConvertion()),
                      std::forward_as_tuple(converter));
  std::cout << "[FIXP (PM)]" << std::endl
            << " - Loading " << converter->getProtocolConvertion()
            << " converter...done!" << std::endl;
}

std::vector<std::string> PluginManager::installMapping(std::string uri)
{
  std::vector<std::string> ret;
  for (auto item : _protocols) {
    if(item.first.compare(extractSchema(uri)) != 0) {
      ret.push_back(item.second->installMapping(uri));
    }
  }

  return ret;
}

boost::shared_ptr<PluginProtocol> PluginManager::getProtocolPlugin(std::string protocol)
{
  return _protocols.find(protocol)->second;
}

boost::shared_ptr<PluginConverter> PluginManager::getConverterPlugin(std::string o_uri, std::string f_uri)
{
  std::string converter = extractSchema(o_uri) + "-to-" + extractSchema(f_uri);
  if(_converters.find(converter) != _converters.end()) {
    return _converters.find(converter)->second;
  } else {
    return boost::shared_ptr<PluginConverter>();
  }
}
