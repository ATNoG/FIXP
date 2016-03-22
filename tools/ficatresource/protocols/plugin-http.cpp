/** Brief: HTTP protocol plugin
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

#include "plugin-http.hpp"

#include <curl/curl.h>
#include <curl/multi.h>
#include <fstream>
#include <iostream>
#include <tuple>

#define SCHEMA "http://"

extern "C" HttpPlugin* create_plugin_object()
{
  return new HttpPlugin;
}

extern "C" void destroy_object(HttpPlugin* object)
{
  delete object;
}

///////////////////////////////////////////////////////////////////////////////
size_t getHttpContent(const void *content, const size_t size, const size_t nmemb, std::string *data)
{
  size_t content_size = size * nmemb;

  std::string tmp((const char*) content, content_size);
  data->append(tmp);

  return content_size;
}

const std::tuple<const std::string, const std::string> requestHttpUri(const std::string uri)
{
  CURL *curl;
  CURLcode res;

  std::string type;
  std::string content;

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getHttpContent);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
      std::cerr << "(HTTP Protocol) Unable to get " << uri << "[Error "
                << curl_easy_strerror(res) << "]" << std::endl << std::flush;
    }

    char* t;
    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &t);
    if(res != CURLE_OK || !t) {
      std::cerr << "(HTTP Protocol) Unable to get content type for" << uri
                << "[Error " << curl_easy_strerror(res) << "]"
                << std::endl << std::flush;
      type = "";
    } else {
      type = t;
    }

    curl_easy_cleanup(curl);
  }

  return std::make_tuple(type.substr(0, type.find(";")), content);
}
///////////////////////////////////////////////////////////////////////////////

void HttpPlugin::processUri(const std::string uri)
{
  std::tuple<std::string, std::string> contentTuple = requestHttpUri(uri);
  std::string content = std::get<1>(contentTuple);

  size_t n = fwrite(content.c_str(), sizeof(char), content.size(), stdout);
  if(content.size() != n) {
    std::cerr << "Error while writing to stdout. ";
  }
}

