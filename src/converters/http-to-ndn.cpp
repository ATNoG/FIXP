/** Brief: HTTP to NDN converter plugin
 *  Copyright (C) 2016
 *  Jose Quevedo <quevedo@av.it.pt>
 *  Carlos Guimaraes <cguimaraes@av.it.pt>
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

#include "http-to-ndn.hpp"

#include <iostream>
#include <regex>

extern "C" HttpToNdnConverter* create_plugin_object()
{
  return new HttpToNdnConverter();
}

extern "C" void destroy_object(HttpToNdnConverter* object)
{
  delete object;
}

std::vector<std::string>
HttpToNdnConverter::extractUrisFrom(MetaMessage& in)
{
  std::vector<std::string> ret;
  std::string content = in._contentPayload;

  std::smatch match;
  std::regex expression("(href|src)=\"(.*?)\"");
  while(std::regex_search(content, match, expression)) {
    std::cout << "[HTTP-to-NDN Converter Plugin]" << std::endl
              << " - Found URI in content: " << match[2].str()
              << std::endl;
    ret.push_back(match[2].str());

    content = match.suffix().str();
  }

  return ret;
}

std::string HttpToNdnConverter::uriToAbsoluteForm(std::string uri, std::string parent)
{
  std::string ret;
  if(uri[0] == '/') {
    ret.append(parent.substr(0, parent.find("/", std::string(HTTP_SCHEMA).size())))
       .append(uri);
  } else {
    ret.append(parent).append(ret[ret.size() - 1] == '/' ? "" : "/").append(uri);
  }
  return ret;
}

std::string
HttpToNdnConverter::convertContent(MetaMessage& in,
                                       std::vector<std::string>& uris,
                                       std::map<std::string, std::string>& mappings)
{
  // Adapt each URI to cope with foreign network
  std::string content = in._contentPayload;
  for(auto item : uris) {
    if(item.find("://") == std::string::npos) {
      // Relative URIs are not changed
      continue;
    }

    // Replace URI on the content
    std::string f_uri = "\"" + item + "\"";
    f_uri.replace(1, std::string(HTTP_SCHEMA).size(), std::string(NDN_SCHEMA));

    for(size_t pos = 0;
        (pos = content.find("\"" + item + "\"", pos)) != std::string::npos;
        pos += f_uri.size()) {
      content.replace(pos, item.size() + 2, f_uri);
    }
  }

  content += "\0";
  return content;
}
