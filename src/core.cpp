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

#include "core.hpp"

#include <boost/log/trivial.hpp>

void Core::loadProtocol(std::string path)
{
  pm.loadProtocol(path, _queue, _tp);
}

void Core::loadConverter(std::string path)
{
  pm.loadConverter(path);
}

void Core::createMapping(std::string uri)
{
  if(_mappings.find(uri) != _mappings.end())
    return;

  std::vector<std::string> f_uris = pm.installMapping(uri);
  for(auto f_uri : f_uris) {
    //FIXME: handle empty strings
    BOOST_LOG_TRIVIAL(info) << "[FIXP (Core)]" << std::endl
                            << " - New mapping: " << f_uri << " -> "
                            << uri << std::endl;
    _mappings.emplace(uri, f_uri);
    _mappings.emplace(f_uri, uri);
  }
}

void Core::stop()
{
  isRunning = false;
  pm.stop();
  _queue.stop();
}

void Core::start()
{
  isRunning = true;

  MetaMessage* in;
  while(isRunning) {
    // Process next message
    try {
      in = _queue.pop();
    } catch(...) {
      return;
    }

    BOOST_LOG_TRIVIAL(trace) << "[FIXP (Core)]" << std::endl
                             << " - Processing next message in the queue ("
                             << in->_uri << ")" << std::endl;

    // Schedule message processing
    std::function<void()> func(std::bind(&Core::processMessage, this, in));
    _tp.schedule(std::move(func));
  }
}

void Core::processMessage(MetaMessage* msg)
{
    MetaMessage* out = new MetaMessage();
    std::map<std::string, std::string>::iterator it;
    if((it = _mappings.find(msg->_uri)) != _mappings.end()) {
      out->_uri = it->second;
    } else {
      BOOST_LOG_TRIVIAL(warning) << "[FIXP (Core)]" << std::endl
                                 << " - Mapping for " << msg->_uri
                                 << " not found" << std::endl;
      return;
    }

    // Extract existent URIs and create mappings to other architectures
    std::vector<std::string> uris;
    uris = pm.getConverterPlugin(msg->_uri, out->_uri)->extractUrisFrom(*msg);

    for(auto item : uris) {
      // Use absolute URIs
      std::string o_uri = item;
      if(item.find("://") == std::string::npos) {
        o_uri = pm.getConverterPlugin(msg->_uri, out->_uri)->uriToAbsoluteForm(item, msg->_uri);
      }

     createMapping(o_uri);
    }

    // Adapt URIs in the content to cope with the destination architecture
    out->_contentPayload = pm.getConverterPlugin(msg->_uri, out->_uri)->convertContent(*msg, uris, _mappings);

    // Send message to destination network architecture
    pm.getProtocolPlugin(out->_uri.substr(0, out->_uri.find("://")))->sendMessage(out);

    // Release the kraken
    delete msg;
}
