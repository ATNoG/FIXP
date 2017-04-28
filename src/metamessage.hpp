/** Brief: Plugin Protocol Interface
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

#ifndef META_MESSAGE__HPP_
#define META_MESSAGE__HPP_

#include "utils.hpp"
#include "uri.hpp"

#include <map>
#include <string>
#include <vector>

enum MetadataMessageType {
  MESSAGE_TYPE_UNKNOWN    = -1,
  MESSAGE_TYPE_REQUEST    =  0,
  MESSAGE_TYPE_RESPONSE   =  1,
  MESSAGE_TYPE_INDICATION =  2
};

class Content {
public:
  Content() { };
  Content(const std::string type, const std::string data)
    : _type(type), _data(data)
  { };

  std::string _type;
  std::string _data;
};

class MetaMessage
{
private:
  Uri _uri;
  std::map<std::string, std::string> _metadata;
  Content _content;

public:
  MetaMessage()
  { };

  MetaMessage(const std::string uri,
              const std::string contentType, const std::string contentData)
    : _uri(uri), _content(contentType, contentData)
  { }

  MetaMessage(MetaMessage*& rhs)
  {
    _uri = rhs->_uri;
    _metadata = rhs->_metadata;
    _content = rhs->_content;
  }

  void setContent(const std::string type, const std::string data)
  {
    _content._type = type;
    _content._data = data;
  }

  Uri getUri() const
  {
    return _uri;
  }

  std::string getUriString() const
  {
    return _uri.toString();
  }

  std::string getEncodedUriString() const
  {
    return _uri.toUriEncodedString();
  }

  void setUri(const Uri uri)
  {
    _uri = uri;
  }

  void setUri(const std::string uri)
  {
    _uri.setUri(uri);
  }

  std::map<std::string, std::string> getMetadata() const
  {
    return _metadata;
  }

  void setMetadata(const std::map<std::string, std::string> metadata)
  {
    _metadata = metadata;
  }

  bool getKeepSession() const
  {
    std::string value;
    try {
      value = _metadata.at("KeepSession");
    } catch(const std::out_of_range& e) {
      return false;
    }

    if(value == "True") {
      return true;
    } else {
      return false;
    }
  }

  void setKeepSession(const bool val)
  {
    if(val) {
      _metadata.emplace("KeepSession", "True");
    } else {
      _metadata.emplace("KeepSession", "False");
    }
  }

  MetadataMessageType getMessageType() const
  {
    std::string messageType;

    try {
      messageType = _metadata.at("MessageType");
    } catch(const std::out_of_range& e) {
      return MESSAGE_TYPE_UNKNOWN;
    }

    std::vector<std::string> typeStr = {"request", "response", "indication"};
    for(int i = 0; i < typeStr.size(); ++i) {
      if(messageType == typeStr[i]) {
        return (MetadataMessageType) i;
      }
    }

    return MESSAGE_TYPE_UNKNOWN;
  }

  void setMessageType(const MetadataMessageType type)
  {
    if(type == MESSAGE_TYPE_REQUEST
       || type == MESSAGE_TYPE_RESPONSE
       || type == MESSAGE_TYPE_INDICATION) {
      std::vector<std::string> typeStr = {"request", "response", "indication"};

      _metadata.emplace("MessageType", typeStr[type]);
    }
  }

  std::string getContentData() const
  {
    return _content._data;
  }

  std::string getContentType() const
  {
    return _content._type;
  }

  void setContentType(const std::string type)
  {
    _content._type = type;
  }
};

#endif /* META_MESSAGE__HPP_ */

