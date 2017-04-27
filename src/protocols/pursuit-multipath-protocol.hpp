/** Brief: FP7 PURSUIT (Blackadder) protocol plugin
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

#ifndef FP7_PURSUIT_PROTOCOL__HPP_
#define FP7_PURSUIT_PROTOCOL__HPP_

#include "../plugin-protocol.hpp"
#include "concurrent-blocking-queue.hpp"
#include "thread-pool.hpp"

#include <thread>
#include <blackadder.hpp>

#define SCHEMA "pursuit-multipath"
#define PURSUIT_ID_LEN_HEX_FORMAT 2 * PURSUIT_ID_LEN
#define DEFAULT_SCOPE "4141414141414141"
#define CHUNK_SIZE 4400

class PcrEntry; // Class definition below

class PursuitMultipathProtocol : public PluginProtocol
{
private:
  Blackadder *ba;
  std::thread _msg_receiver;
  std::thread _msg_sender;
  std::map<std::string, std::vector<PcrEntry> > pending_chunk_requests;

public:
  PursuitMultipathProtocol(ConcurrentBlockingQueue<const MetaMessage*>& queue,
                  ThreadPool& tp);
  ~PursuitMultipathProtocol();

  void start();
  void stop();

  std::string getProtocol() const { return SCHEMA; };
  std::string installMapping(const std::string uri);

protected:
  void processMessage(const MetaMessage* msg);

private:
  void startReceiver();
  void startSender();

  int publishScope(const std::string name, unsigned char strategy);
  int subscribeScope(const std::string name, unsigned char strategy);
  int publishInfo(const std::string name, unsigned char strategy);

  int publish_data(const Uri uri, unsigned char strategy, unsigned char* fid, void* content, size_t content_size);
  int publishUriContent(const Uri uri, void* content, size_t content_size);
  int subscribeUri(const Uri uri);
  int unsubscribeUri(const Uri uri);
};

class PcrEntry
{
private:
  std::string _chunk_uri;
  unsigned char _fid;
  char* _rfid;

public:
  PcrEntry(std::string chunk_uri, unsigned char fid, char* rfid)
  {
    _chunk_uri = chunk_uri;
    _fid = fid;
    _rfid = rfid;
  }

  std::string getChunkUri() const
  {
    return _chunk_uri;
  }

  size_t getChunkNumber() const
  {
    return strtoull(_chunk_uri.substr(_chunk_uri.size() - PURSUIT_ID_LEN_HEX_FORMAT,
                                      PURSUIT_ID_LEN_HEX_FORMAT).c_str(), NULL, 16);
  }

  unsigned char getFid() const
  {
    return _fid;
  }

  char* getReverseFid() const
  {
    return _rfid;
  }
};

#endif /* FP7_PURSUIT_PROTOCOL__HPP_ */

