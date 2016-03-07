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

#include "http-protocol.hpp"

#include <curl/curl.h>
#include <curl/multi.h>
#include <iostream>

extern "C" HttpProtocol* create_plugin_object(ConcurrentBlockingQueue<MetaMessage*>& queue,
                                              ThreadPool& tp)
{
  return new HttpProtocol(queue, tp);
}

extern "C" void destroy_object(HttpProtocol* object)
{
  delete object;
}

///////////////////////////////////////////////////////////////////////////////
std::string createForeignUri(std::string o_uri)
{
  //TODO
  return "";
}

size_t getHttpContent(void *content, size_t size, size_t nmemb, std::string *data)
{
  size_t content_size = size * nmemb;

  std::string tmp((const char*) content, content_size);
  data->append(tmp);

  return content_size;
}

std::tuple<std::string, std::string> requestHttpUri(std::string uri)
{
  std::cout << "[HTTP Protocol Plugin]" << std::endl
            << " - Requesting" << uri << std::endl;

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
      std::cout << "[HTTP Protocol Plugin]" << std::endl
                << "[" << curl_easy_strerror(res)
                << "] Unable to get " << uri << std::endl;
    }

    char* t;
    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &t);
    if(res != CURLE_OK || !t) {
      std::cout << "[HTTP Protocol Plugin]" << std::endl
                << "[" << curl_easy_strerror(res)
                << "] Unable to get content type for " << uri << std::endl;

      type = "";
    } else {
      type = t;
    }

    curl_easy_cleanup(curl);
  }

  return std::make_tuple(type.substr(0, type.find(";")), content);
}
///////////////////////////////////////////////////////////////////////////////

HttpProtocol::HttpProtocol(ConcurrentBlockingQueue<MetaMessage*>& queue,
                           ThreadPool& tp)
    : PluginProtocol(queue, tp)
{ }

HttpProtocol::~HttpProtocol()
{ }

void HttpProtocol::stop()
{
  isRunning = false;
  _msg_to_send.stop();

  _msg_receiver.join();
  _msg_sender.join();
}

void HttpProtocol::start()
{
  isRunning = true;

  _msg_receiver = std::thread(&HttpProtocol::startReceiver, this);
  _msg_sender = std::thread(&HttpProtocol::startSender, this);
}

std::string HttpProtocol::installMapping(std::string uri)
{
  std::string f_uri = createForeignUri(uri);

  // Remove schema from uri
  if(f_uri.find(SCHEMA) == std::string::npos) {
    std::cout << "[HTTP Protocol Plugin]" << std::endl
              << " - Foreign URI (" << f_uri
              << ") with an invalid schema"
              << "(Original: " << uri << ")" << std::endl;
    return "";
  }

  return f_uri;
}

void HttpProtocol::startReceiver()
{
  //TODO
}

void HttpProtocol::startSender()
{
  MetaMessage* out;
  while(isRunning) {
    try {
      out = _msg_to_send.pop();
    } catch(...) {
      return;
    }

    std::cout << "[HTTP Protocol Plugin]"  << std::endl
              << " - Processing next message in the queue ("
              << out->getUri() << ")" << std::endl;

    std::function<void()> func(std::bind(&HttpProtocol::processMessage, this, out));
    _tp.schedule(std::move(func));
  }
}

void HttpProtocol::processMessage(MetaMessage* msg)
{
  // Request content from the original network
  std::tuple<std::string, std::string> content = requestHttpUri(msg->getUri());

  // Send received response to Core
  MetaMessage* response = new MetaMessage();
  response->setUri(msg->getUri());
  response->setContent(std::get<0>(content), std::get<1>(content));

  std::cout << "[HTTP Protocol Plugin]" << std::endl
            << " Retrieving the response of " << msg->getUri()
            << " to FIXP" << std::endl;
  receivedMessage(response);

  // Release the kraken
  delete msg;
}

