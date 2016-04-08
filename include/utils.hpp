/** Brief: Utility and auxiliary functions
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

#ifndef UTILS__HPP_
#define UTILS__HPP_

#include "logger.hpp"

#include <algorithm>
#include <cctype>
#include <magic.h>
#include <stdlib.h>
#include <string>

inline
std::string discoverContentType(const std::string content)
{
  magic_t magic = magic_open(MAGIC_MIME_TYPE | MAGIC_CONTINUE | MAGIC_CHECK);
  if(magic == NULL) {
    FIFU_LOG_ERROR("(utils) Error while opening libmagic");
    return "";
  }

  if(magic_load(magic, NULL) != 0) {
    FIFU_LOG_ERROR("(utils) Error while loading the default database");
    magic_close(magic);
    return "";
  }

  const char* ct = magic_buffer(magic, content.data(), content.size());
  if(ct == NULL) {
    FIFU_LOG_ERROR("(utils) Error while detecting the file type");
    magic_close(magic);
    return "";
  }

  std::string contentType = ct;
  magic_close(magic);

  return contentType;
}

inline
std::string trimString(std::string str)
{
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());

  return str;
}

inline
std::string unescapeString(const std::string str)
{
  std::string res = str;

  // Unescape string
  size_t pos = 0;
  while((pos = res.find("%", pos)) != std::string::npos) {
    res.replace(pos, 3, 1, (char)(int)strtol(res.substr(pos + 1, 2).c_str(), NULL, 16));
  }

  return res;
}

#endif /* UTILS__HPP_ */

