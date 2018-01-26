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
#include "logger.hpp"
#include "utils.hpp"

#include <algorithm>
#include <memory>

void Core::loadProtocol(const std::string path)
{
  pm.loadProtocol(path, _queue, _tp);
}

void Core::loadConverter(const std::string path)
{
  pm.loadConverter(path);
}

std::vector<Uri> Core::createMapping(const Uri o_uri)
{
  std::vector<Uri> f_uris;
  std::vector<std::string> schemes = pm.getSupportedSchemas();

  // Drop the scheme of the original URI
  auto it = std::find(schemes.begin(), schemes.end(), o_uri.getSchema());
  if (it != schemes.end()) {
    schemes.erase(it);
  }

  // Get existing mappings for the given URI
  std::unique_lock<std::shared_timed_mutex> lock(_mappings_mutex);
  for(auto& item : _mappings) {
    if(item.second == o_uri) {
      f_uris.push_back(item.first);

      // Drop the scheme from the list
      it = std::find(schemes.begin(), schemes.end(), item.first.getSchema());
      if (it != schemes.end()) {
        schemes.erase(it);
      }
    }
  }

  // Check if all protocols have a mapping for the given URI
  // If no mapping for a protocol is found create one
  for(auto& scheme : schemes) {
    Uri f_uri = pm.installMapping(o_uri, scheme);
    FIFU_LOG_INFO("(Core) New mapping: " + f_uri.toString() + " -> " + o_uri.toString());

    _mappings.emplace(f_uri, o_uri);
    f_uris.push_back(f_uri);
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

  const MetaMessage* in;
  while(isRunning) {
    // Process next message
    try {
      in = _queue.pop();
    } catch(...) {
      return;
    }

    // Schedule message processing
    FIFU_LOG_INFO("(Core) Scheduling next message (" + in->getUriString() + ") processing");
    std::function<void()> func(std::bind(&Core::processMessage, this, in));
    _tp.schedule(std::move(func));
  }
}

void Core::processMessage(const MetaMessage* msg)
{
  FIFU_LOG_INFO("(Core) Processing message (" + msg->getUriString() + ")");
  std::vector<Uri> out_uris;

  // Begin: Locking scope
  {
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
        std::vector<Uri> vec;
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

      if(!msg->getKeepSession()) {
        _waiting_for_response.erase(msg->getUri());
      }
      lock_wait.unlock();
    }
  } // End: Locking scope

  if(out_uris.size() == 0) {
    FIFU_LOG_WARN("(Core) Mapping for " + msg->getUriString() + " not found!");

    delete msg;
    return;
  }

  for(auto& item : out_uris) {
    MetaMessage* out = new MetaMessage();
    out->setUri(item);
    out->setMetadata(msg->getMetadata());

    std::string contentType = msg->getContentType();
    if(contentType == "") {
      contentType = discoverContentType(msg->getContentData());
      FIFU_LOG_WARN("(Core) Detected content type (" + contentType +") of " + msg->getUriString());
    }

    std::shared_ptr<PluginConverter> converter = pm.getConverterPlugin(contentType);
    if(converter) {
      // Extract existent URIs and create mappings to other architectures
      std::map<std::string, Uri> uris;
      uris = converter->extractUrisFromContent(msg->getUri(), msg->getContentData());

      std::map<std::string, Uri> mappings_for_convertion;
      for(auto& o_uri : uris) {
        std::vector<Uri> f_uris = createMapping(o_uri.second);

        for(auto& f_uri : f_uris) {
          if(f_uri.getSchema() == out->getUri().getSchema()) {
            mappings_for_convertion.emplace(o_uri.first, f_uri);
            break;
          }
        }
      }

      out->setContent(contentType,
                      converter->convertContent(msg->getContentData(),
                                                mappings_for_convertion));
    } else {
      // If no converter is found send the content without conversion
      out->setContent(contentType, msg->getContentData());
    }

    // Send message to destination network architecture
    std::shared_ptr<PluginProtocol> protocol = pm.getProtocolPlugin(out->getUri().getSchema());
    if(protocol) {
      protocol->sendMessage(out);
    } else {
      FIFU_LOG_WARN("(Core) Protocol endpoint for '" + out->getUri().getSchema() + "' not found");
    }
  }

  delete msg;
}
