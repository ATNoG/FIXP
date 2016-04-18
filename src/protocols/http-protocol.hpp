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

#ifndef HTTP_PROTOCOL__HPP_
#define HTTP_PROTOCOL__HPP_

#include "../plugin-protocol.hpp"
#include "concurrent-blocking-queue.hpp"
#include "thread-pool.hpp"

#include <microhttpd.h>
#include <thread>

#define SCHEMA "http"

#define DEFAULT_HOSTNAME "127.0.0.1"
#define HTTPD_PORT 8000

class HttpProtocol : public PluginProtocol
{
private:
  struct MHD_Daemon* daemon;
  std::thread _msg_receiver;
  std::thread _msg_sender;

  std::map<Uri, MHD_Connection*> pendingRequests;

public:
  HttpProtocol(ConcurrentBlockingQueue<const MetaMessage*>& queue,
               ThreadPool& tp);
  ~HttpProtocol();

  void start();
  void stop();

  std::string getProtocol() const { return SCHEMA; };
  std::string installMapping(const std::string uri);

protected:
  void processMessage(const MetaMessage* msg);

private:
  void startReceiver();
  void startSender();

  void sendMessage(const MetaMessage* out);

  static int static_answer_to_connection(void *cls, struct MHD_Connection *connection,
                                         const char *url,
                                         const char *method, const char *version,
                                         const char *upload_data,
                                         size_t *upload_data_size, void **con_cls);

  int answer_to_connection(struct MHD_Connection *connection,
                           const char *url,
                           const char *method, const char *version,
                           const char *upload_data,
                           size_t *upload_data_size, void **con_cls);

  void responseHttpUri(const MetaMessage* msg);
};

#endif /* HTTP_PROTOCOL__HPP_ */

