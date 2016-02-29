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

#include <boost/log/trivial.hpp>
#include <curl/curl.h>
#include <curl/multi.h>

extern "C" HttpProtocol* create_plugin_object(ConcurrentBlockingQueue<MetaMessage*>& queue)
{
  return new HttpProtocol(queue);
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

CURL* newConnection(std::string uri, std::string& content)
{
  BOOST_LOG_TRIVIAL(trace) << "[HTTP Protocol Plugin]" << std::endl
                           << " - Creating connection to request "
                           << uri << std::endl;

  CURL* connection;
  connection = curl_easy_init();
  if(connection) {
    curl_easy_setopt(connection, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(connection, CURLOPT_WRITEFUNCTION, getHttpContent);
    curl_easy_setopt(connection, CURLOPT_WRITEDATA, &content);
  }

  return connection;
}

size_t checkConnectionStatus(CURLM*& cm)
{
  int pendingRequests = -1;

  CURLMcode mc;
  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;

  int maxfd = -1;
  FD_ZERO(&fdread);
  FD_ZERO(&fdwrite);
  FD_ZERO(&fdexcep);

  mc = curl_multi_fdset(cm, &fdread, &fdwrite, &fdexcep, &maxfd);
  if(mc != CURLM_OK) {
    BOOST_LOG_TRIVIAL(error) << "[Error Code " << mc
                             << "] curl_multi_fdset() failed";
    return -1;
  }

  // FIXME:
  struct timeval timeout = { 0, 200 * 1000 }; // Timeout: 200 ms
  do {
    if(select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout) != -1) {
      curl_multi_perform(cm, &pendingRequests);
    }
  } while(pendingRequests != 0);

  return pendingRequests;
}
///////////////////////////////////////////////////////////////////////////////

HttpProtocol::HttpProtocol(ConcurrentBlockingQueue<MetaMessage*>& queue)
    : PluginProtocol(queue)
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
    BOOST_LOG_TRIVIAL(warning) << "[HTTP Protocol Plugin]" << std::endl
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
  std::map<CURL*, MetaMessage*> connections;
  CURLM* cm;

  cm = curl_multi_init();

  MetaMessage* msg;
  while(isRunning) {
    try {
      msg = _msg_to_send.pop();
    } catch(...) {
      return;
    }

    BOOST_LOG_TRIVIAL(trace) << "[HTTP Protocol Plugin]"  << std::endl
                             << " - Processing next message in the queue ("
                             << msg->_uri << ")" << std::endl;

    MetaMessage* out = new MetaMessage();
    out->_uri = msg->_uri;

    // Create new connection to send incoming message
    CURL* connection = newConnection(msg->_uri, out->_contentPayload);
    connections.emplace(connection, out);
    curl_multi_add_handle(cm, connection);

    // Perform HTTP request
    int tmp = -1;
    curl_multi_perform(cm, &tmp);

    // Release the kraken
    delete msg;

    // Process responses to pending requests
    // FIXME: handle errors
    if(checkConnectionStatus(cm) == connections.size()) {
      continue;
    }

    int msgs_left;
    CURLMsg *cmsg;
    while((cmsg = curl_multi_info_read(cm, &msgs_left))) {
      if(cmsg->msg != CURLMSG_DONE) {
        continue;
      }

      std::map<CURL*, MetaMessage*>::iterator it;
      if((it = connections.find(cmsg->easy_handle)) != connections.end()) {
        BOOST_LOG_TRIVIAL(trace) << "[HTTP Protocol Plugin]" << std::endl
                                 << " Retrieving the response of " << it->second->_uri
                                 << " to FIXP" << std::endl;

        receivedMessage(it->second);
        connections.erase(it);

        // Clean up
        curl_easy_cleanup(it->first);
      }
    }
  }

  // Clean up
  curl_multi_cleanup(cm);
}

