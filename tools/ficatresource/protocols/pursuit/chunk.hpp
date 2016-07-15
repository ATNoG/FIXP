/** Brief: FP7 PURSUIT (Blackadder) protocol chunk
 *         as defined in FP7 PURSUIT Deliverable D3.5
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

#ifndef PURSUIT_CHUNK__HPP_
#define PURSUIT_CHUNK__HPP_

#include <blackadder.hpp>

#define CHUNK_REQUEST 201
#define CHUNK_RESPONSE 202

class Chunk
{
public:
  size_t size() const
  {
    return _size;
  }

  const unsigned char getType() const
  {
    return _type;
  }

  const unsigned char getPathId() const
  {
    return _path_id;
  }

protected:
  unsigned char _type;
  unsigned char _path_id;
  size_t        _size;
};

class ChunkRequest : public Chunk
{
public:
  ChunkRequest(const char* data);
  ChunkRequest(const char* reverse_fid, unsigned char path_id);

  ~ChunkRequest() { }

  size_t toBytes(char* data);

  char* getReverseFid()
  {
    return _reverse_fid;
  }

private:
  char _reverse_fid[FID_LEN];
};

class ChunkResponse : public Chunk
{
private:
  char*  _payload;
  size_t _sender_wnd;
  size_t _payload_len;

public:
  ChunkResponse(const char* data, size_t size);
  ChunkResponse(const char* payload, size_t payload_len, unsigned char path_id, size_t sender_wnd);

  ~ChunkResponse() { }

  size_t toBytes(char* data);

  const char* getPayload() const
  {
    return _payload;
  }

  size_t getPayloadLen() const
  {
    return _payload_len;
  }

  size_t getSenderWnd() const
  {
    return _sender_wnd;
  }
};

#endif /* PURSUIT_CHUNK__HPP_ */

