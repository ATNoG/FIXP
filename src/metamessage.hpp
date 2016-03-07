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

#include <string>

class Content {
public:
  Content() { };
  Content(std::string type, std::string data)
    : _type(type), _data(data)
  { };

  std::string _type;
  std::string _data;
};

class MetaMessage
{
private:
  std::string _uri;
  std::string _metadata;
  Content _content;

public:
  MetaMessage()
  { };

  MetaMessage(std::string uri, std::string metadata, std::string contentType, std::string contentData)
    : _uri(uri), _metadata(metadata), _content(contentType, contentData)
  { }

  MetaMessage(MetaMessage*& rhs)
  {
    _uri = rhs->_uri;
    _metadata = rhs->_metadata;
    _content = rhs->_content;
  }

  void setContent(std::string type, std::string data)
  {
    _content._type = type;
    _content._data = data;
  }

  std::string getUri()
  {
    return _uri;
  }

  void setUri(std::string uri)
  {
    _uri = uri;
  }

  std::string getMetadata()
  {
    return _metadata;
  }

  void setMetadata(std::string metadata)
  {
    _metadata = metadata;
  }

  std::string getContentData()
  {
    return _content._data;
  }

  std::string getContentType()
  {
    return _content._type;
  }

  void setContentType(std::string type)
  {
    _content._type = type;
  }
};

#endif /* META_MESSAGE__HPP_ */

