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
#include "logger.hpp"

#include <curl/curl.h>
#include <curl/multi.h>
#include <string.h>

extern "C" HttpProtocol* create_plugin_object(ConcurrentBlockingQueue<const MetaMessage*>& queue,
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
  Uri uri(o_uri);

  return std::string(SCHEMA) + ":"
          + "//" + DEFAULT_HOSTNAME
          + (HTTPD_PORT == 80 ? "" : ":" + std::to_string(HTTPD_PORT)) + "/"
          + (uri.getAuthority().size() != 0 ? "/" + uri.getAuthority() : "")
          + uri.getPath()
          + (uri.getQuery().size() != 0 ? "?" + uri.getQuery() : "")
          + (uri.getFragment().size() != 0 ? "#" + uri.getFragment() : "");
}
///////////////////////////////////////////////////////////////////////////////

size_t getHttpContent(const void *content, const size_t size, const size_t nmemb, std::string *data)
{
  size_t content_size = size * nmemb;

  std::string tmp((const char*) content, content_size);
  data->append(tmp);

  return content_size;
}

bool requestHttpUri(const std::string uri, std::string& type, std::string& content)
{
  FIFU_LOG_INFO("(HTTP Protocol) Requesting " + uri);

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(!curl) {
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getHttpContent);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30 seconds

  res = curl_easy_perform(curl);
  if(res != CURLE_OK) {
    FIFU_LOG_INFO("(HTTP Protocol) Unable to get " + uri
                  + "[Error " + curl_easy_strerror(res) + "]");
    return false;
  }

  char* t;
  res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &t);
  if(res != CURLE_OK || !t) {
    FIFU_LOG_WARN("(HTTP Protocol) Unable to get content type for " + uri
                  + "[Error " + curl_easy_strerror(res) + "]");
  } else {
    type = t;
    type = type.substr(0, type.find(";"));
  }

  curl_easy_cleanup(curl);
  return true;
}
///////////////////////////////////////////////////////////////////////////////

HttpProtocol::HttpProtocol(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                           ThreadPool& tp)
    : PluginProtocol(queue, tp)
{ }

HttpProtocol::~HttpProtocol()
{ }

void HttpProtocol::stop()
{
  isRunning = false;

  MHD_stop_daemon(daemon);
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

std::string HttpProtocol::installMapping(const std::string uri)
{
  return createForeignUri(uri);
}

void HttpProtocol::startReceiver()
{
  daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_POLL | MHD_USE_SUSPEND_RESUME,
                            HTTPD_PORT, NULL, NULL,
                            &HttpProtocol::static_answer_to_connection, this, MHD_OPTION_END);
  if(daemon == NULL) {
    FIFU_LOG_ERROR("Unable to start HTTP server capabilities");
    return;
  }
}

void HttpProtocol::startSender()
{
  const MetaMessage* out;
  while(isRunning) {
    try {
      out = _msg_to_send.pop();
    } catch(...) {
      return;
    }

    // Schedule message processing
    FIFU_LOG_INFO("(HTTP Protocol) Scheduling next message (" + out->getUriString() + ") processing");
    std::function<void()> func(std::bind(&HttpProtocol::processMessage, this, out));
    _tp.schedule(std::move(func));
  }
}

void HttpProtocol::processMessage(const MetaMessage* msg)
{
  FIFU_LOG_INFO("(HTTP Protocol) Processing message (" + msg->getUriString() + ")");

  if(msg->getMessageType() == MESSAGE_TYPE_REQUEST) {
    // Request content from the original network
    std::string content;
    std::string type;
    if(!requestHttpUri(msg->getUriString(), type, content)) {
      // Release the kraken and return
      delete msg;
      return;
    }

    // Send received response to Core
    MetaMessage* response = new MetaMessage();
    response->setUri(msg->getUri());
    response->setMessageType(MESSAGE_TYPE_RESPONSE);
    response->setContent(type, content);

    FIFU_LOG_INFO("(HTTP Protocol) Received response of " + msg->getUriString());
    receivedMessage(response);
  } else if(msg->getMessageType() == MESSAGE_TYPE_RESPONSE) {
    responseHttpUri(msg);
  }

  // Release the kraken
  delete msg;
}

int HttpProtocol::static_answer_to_connection(void *cls, struct MHD_Connection *connection,
                                              const char *url,
                                              const char *method, const char *version,
                                              const char *upload_data,
                                              size_t *upload_data_size, void **con_cls)
{
  static int dummy;
  if(strcmp(method, "GET") != 0) {
    return MHD_NO;
  }

  if(&dummy != *con_cls) {
    *con_cls = &dummy;
    return MHD_YES;
  }

  if(*upload_data_size != 0) {
    return MHD_NO;
  }
  *con_cls = NULL;

  HttpProtocol* instance = (HttpProtocol*) cls;
  return instance->answer_to_connection(connection, url, method, version, upload_data, upload_data_size, con_cls);
}

int HttpProtocol::answer_to_connection(struct MHD_Connection *connection,
                                       const char *url,
                                       const char *method, const char *version,
                                       const char *upload_data,
                                       size_t *upload_data_size, void **con_cls)
{
  const char* host = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host");
  if(host == NULL) {
    FIFU_LOG_WARN("(HTTP Protocol) Unable to find host parameter. Replying with Bad Request (400) error message...");
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(0, (void*) "",
                                               MHD_RESPMEM_PERSISTENT);

    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    return ret;
  }

  MetaMessage* in = new MetaMessage();
  in->setUri(std::string(SCHEMA) + ":" + "//" + host + url);
  in->setMessageType(MESSAGE_TYPE_REQUEST);

  FIFU_LOG_INFO("(HTTP Protocol) Received GET request to " + in->getUriString());
  pendingConnections.emplace(in->getUri(), connection);
  MHD_suspend_connection(connection);

  receivedMessage(in);

  return MHD_YES;
}

void HttpProtocol::responseHttpUri(const MetaMessage* msg)
{
  // Get pending connection
  auto it = pendingConnections.find(msg->getUri());
  if(it == pendingConnections.end()) {
    return;
  }

  if(it->second == NULL) {
    return;
  }
  struct MHD_Connection* connection = it->second;
  pendingConnections.erase(it);

  struct MHD_Response* response;
  response = MHD_create_response_from_buffer(msg->getContentData().size(), (void*) msg->getContentData().c_str(), MHD_RESPMEM_PERSISTENT);
  MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, msg->getContentType().c_str());

  MHD_resume_connection(connection);
  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  if(ret == MHD_YES) {
    FIFU_LOG_INFO("(HTTP Protocol) Sending the response message of " + msg->getUriString());
  } else {
    FIFU_LOG_INFO("(HTTP Protocol) Failed to send response message of " + msg->getUriString());
  }

  MHD_destroy_response(response);
}

