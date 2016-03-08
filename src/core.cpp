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

#include <iostream>

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
    std::cout << "[FIXP (Core)]" << std::endl
              << " - New mapping: " << f_uri << " -> "
              << uri << std::endl;

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

    std::cout << "[FIXP (Core)]" << std::endl
              << " - Processing next message in the queue ("
              << in->getUri() << ")" << std::endl;

    // Schedule message processing
    std::function<void()> func(std::bind(&Core::processMessage, this, in));
    _tp.schedule(std::move(func));
  }
}

void Core::processMessage(MetaMessage* msg)
{
    std::vector<std::string> out_uris;

    // Check if message is identified by a foreign URI
    // (i.e., foreign URI exists in mappings)
    auto it = _mappings.find(msg->getUri());
    if(it != _mappings.end()) {
      out_uris.push_back(it->second);

      // Message will (eventually) be replied
      auto it_wait = _waiting_for_response.find(it->second);
      if(it_wait != _waiting_for_response.end()) {
        it_wait->second.push_back(msg->getUri());
      } else {
        std::vector<std::string> vec;
        vec.push_back(msg->getUri());
        _waiting_for_response[it->second] = vec;
      }

    } else {
    // Let's assume that is an original URI
    // (i.e., response to a previous request)
      auto it_wait = _waiting_for_response.find(msg->getUri());
      if(it_wait != _waiting_for_response.end()) {
        out_uris = it_wait->second;
      }

      // FIXME: make it thread-safe
      _waiting_for_response.erase(msg->getUri());
    }

    if(out_uris.size() == 0) {
      std::cout << "[FIXP (Core)]" << std::endl
                << " - Mapping for " << msg->getUri()
                << " not found" << std::endl;

      delete msg;
      return;
    }

    for(auto item : out_uris) {
      MetaMessage* out = new MetaMessage();
      out->setUri(item);

      // Extract existent URIs and create mappings to other architectures
      boost::shared_ptr<PluginConverter> converter = pm.getConverterPlugin(msg->getContentType());
      if(converter) {
        std::map<std::string, std::string> uris;
        uris = converter->extractUrisFromContent(msg->getUri(), msg->getContentData());

        std::map<std::string, std::string> mappings_;
        for(auto& item : uris) {
          createMapping(item.second);

          // Adapt URIs in the content to cope with the destination architecture
          auto it = std::find_if(_mappings.begin(),
                                 _mappings.end(),
                                 [=](std::pair<std::string, std::string> it) {
                                   if(it.second.compare(item.second) == 0
                                      && it.first.find(out->getUri().substr(0, out->getUri().find("://"))) != std::string::npos) {
                                      return true;
                                    } else {
                                      return false;
                                    }
                                  });

          mappings_.emplace(item.first, it->first);
        }

        out->setContent(msg->getContentType(),
                        converter->convertContent(msg->getContentData(),
                                                  mappings_));
      } else {
        // If no converter is found send the content without conversion
        out->setContent(msg->getContentType(), msg->getContentData());
      }

      // Send message to destination network architecture
      pm.getProtocolPlugin(out->getUri().substr(0, out->getUri().find("://")))->sendMessage(out);
    }

    // Release the kraken
    delete msg;
}
