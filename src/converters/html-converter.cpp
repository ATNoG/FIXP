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
#include "utils.hpp"

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

std::map<std::string, Uri>
HtmlConverter::extractUrisFromContent(const Uri uri, const std::string content)
{
  std::map<std::string, Uri> uris;

  std::smatch match;
  std::regex expression("(href|src)[[:space:]]*=[[:space:]]*(\"|'|`)(.*?)\\2",
                        std::regex_constants::ECMAScript | std::regex_constants::icase);

  std::sregex_token_iterator end;
  for(std::sregex_token_iterator i(content.begin(), content.end(), expression, {2, 3}); i != end; ++i) {
    std::string quote = i->str();
    std::string e_uri = (++i)->str();
    std::string trimmed_e_uri = trimString(e_uri);

    FIFU_LOG_INFO("(HTML Converter) Found " + trimmed_e_uri + " resource in " + uri.toString());
    uris.emplace(quote + e_uri + quote,
                 Uri(uri, trimmed_e_uri));
  }

  return uris;
}

std::string
HtmlConverter::convertContent(const std::string content,
                              const std::map<std::string, Uri>& mappings)
{
  // Adapt each URI to cope with foreign network
  std::string tmp = content;
  for(auto& uris : mappings) {
    // Replace URI on the content
    std::string o_uri = uris.first;

    std::string f_uri;
    f_uri.append("\"").append(uris.second.toString()).append("\"");
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

