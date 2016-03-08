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

std::vector<std::string> Core::createMapping(std::string o_uri)
{
  std::vector<std::string> f_uris;

  std::unique_lock<std::shared_timed_mutex> lock(_mappings_mutex);
  for(auto& item : _mappings) {
    if(item.second == o_uri) {
      f_uris.push_back(item.first);
    }
  }

  if(f_uris.size() != 0) {
    return f_uris;
  }

  f_uris = pm.installMapping(o_uri);
  for(auto f_uri : f_uris) {
    //FIXME: handle empty strings
    std::cout << "[FIXP (Core)]" << std::endl
              << " - New mapping: " << f_uri << " -> "
              << o_uri << std::endl;

    _mappings.emplace(f_uri, o_uri);
  }

  return f_uris;
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

    { // Begin: Locking scope

    // Check if message is identified by a foreign URI
    // (i.e., foreign URI exists in mappings)
    std::shared_lock<std::shared_timed_mutex> lock_map(_mappings_mutex);
    auto it = _mappings.find(msg->getUri());
    if(it != _mappings.end()) {
      out_uris.push_back(it->second);
      lock_map.unlock();

      // Message will (eventually) be replied
      std::unique_lock<std::shared_timed_mutex> lock_wait(_waiting_for_response_mutex);
      auto it_wait = _waiting_for_response.find(it->second);
      if(it_wait != _waiting_for_response.end()) {
        it_wait->second.push_back(msg->getUri());
      } else {
        std::vector<std::string> vec;
        vec.push_back(msg->getUri());
        _waiting_for_response[it->second] = vec;
      }
      lock_wait.unlock();

    } else {
      lock_map.unlock();
    // Let's assume that is an original URI
    // (i.e., response to a previous request)
      std::unique_lock<std::shared_timed_mutex> lock_wait(_waiting_for_response_mutex);
      auto it_wait = _waiting_for_response.find(msg->getUri());
      if(it_wait != _waiting_for_response.end()) {
        out_uris = it_wait->second;
      }

      _waiting_for_response.erase(msg->getUri());
      lock_wait.unlock();
    }

    } // End: Locking scope

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

        std::map<std::string, std::string> mappings_for_convertion;
        for(auto& o_uri : uris) {
          std::vector<std::string> f_uris = createMapping(o_uri.second);

          for(auto& f_uri : f_uris) {
            if(f_uri.find(out->getUri().substr(0, out->getUri().find("://"))) != std::string::npos) {
              mappings_for_convertion.emplace(o_uri.first, f_uri);
              break;
            }
          }
        }

        out->setContent(msg->getContentType(),
                        converter->convertContent(msg->getContentData(),
                                                  mappings_for_convertion));
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
