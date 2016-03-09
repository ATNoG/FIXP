/** Brief: HTML converter plugin
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

#include "html-converter.hpp"
#include "logger.hpp"

#include <iostream>
#include <regex>

extern "C" HtmlConverter* create_plugin_object()
{
  return new HtmlConverter();
}

extern "C" void destroy_object(HtmlConverter* object)
{
  delete object;
}

std::map<std::string, std::string>
HtmlConverter::extractUrisFromContent(const std::string uri, const std::string content)
{
  std::map<std::string, std::string> uris;

  std::smatch match;
  std::regex expression("(href|src)=\"(.*?)\"");

  std::sregex_token_iterator end;
  for(std::sregex_token_iterator i(content.begin(), content.end(), expression, 2); i != end; ++i) {
    FIFU_LOG_INFO("(HTML Converter) Found " + i->str() + " resource in " + uri);
    uris.emplace(i->str(), uriToAbsoluteForm(i->str(), uri));
  }

  return uris;
}

std::string HtmlConverter::uriToAbsoluteForm(const std::string uri, const std::string parent)
{
  // URI already is in absolute form
  if(uri.find("://") != std::string::npos)
    return uri;

  std::string ret;
  if(uri[0] == '/') {
    ret.append(parent.substr(0, parent.find("/", parent.find(SCHEMA_DELIMITER) + sizeof(SCHEMA_DELIMITER))))
       .append(uri);
  } else {
    ret.append(parent).append(parent[0] == '/' ? "" : "/").append(uri);
  }

  return ret;
}

std::string
HtmlConverter::convertContent(const std::string content,
                              const std::map<std::string, std::string>& mappings)
{
  // Adapt each URI to cope with foreign network
  std::string tmp = content;
  for(auto& uris : mappings) {
    // Replace URI on the content
    std::string o_uri;
    o_uri.append("\"").append(uris.first).append("\"");

    std::string f_uri;
    f_uri.append("\"").append(uris.second).append("\"");
    for(size_t pos = 0;
        (pos = tmp.find(o_uri, pos)) != std::string::npos;
        pos += f_uri.size()) {
      FIFU_LOG_INFO("(HTML Converter) Replacing " + o_uri + " by " + f_uri)
      tmp.replace(pos, o_uri.size(), f_uri);
    }
  }

  tmp += "\0";
  return tmp;
}

