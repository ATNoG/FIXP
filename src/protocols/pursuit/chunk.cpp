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

#include "chunk.hpp"

ChunkRequest::ChunkRequest(const char* data)
{
  _size = 0;

  // Decode chunk type
  _type = data[_size++];
  if(_type != CHUNK_REQUEST) {
    throw;
  }

  // Decode reverse FID
  memcpy(_reverse_fid, data + _size, FID_LEN);
  _size += FID_LEN;

  // Decode path ID
  _path_id = data[_size++];
}

ChunkRequest::ChunkRequest(const char* reverse_fid,
                           unsigned char path_id)
{
  _size = 0;

  // Set chunk type
  _type = CHUNK_REQUEST;
  _size++;

  // Set reverse FID
  memcpy(_reverse_fid, reverse_fid, FID_LEN);
  _size += FID_LEN;

  // Set path ID
  _path_id = path_id;
  _size++;
}

size_t
ChunkRequest::toBytes(char* data)
{
  size_t pos = 0;

  // Encode chunk type
  data[pos++] = _type;

  // Encode reverse FID
  memcpy(data + pos, _reverse_fid, FID_LEN);
  pos += FID_LEN;

  // Encode path ID
  data[pos++] = _path_id;

  return pos;
}

ChunkResponse::ChunkResponse(const char* data, size_t size)
{
  _size = 0;

  // Decode chunk type
  _type = data[_size++];
  if(_type != CHUNK_RESPONSE) {
    throw;
  }

  // Decode path ID
  _path_id = data[_size++];

  // Decode sender window field size
  size_t swnd_len = data[_size++];

  // Decode sender window size
  char sender_wnd[swnd_len];
  memcpy(sender_wnd, data + _size, swnd_len);
  _sender_wnd = atoi(sender_wnd);
  _size += swnd_len;

  // Decode payload
  _payload_len = size - 3 - swnd_len;
  _payload = new char[_payload_len];
  _payload = (char*) (data + _size);

  _size += _payload_len;
}

ChunkResponse::ChunkResponse(const char* payload, size_t payload_len, unsigned char path_id, size_t sender_wnd)
{
  _size = 0;

  // Set chunk type
  _type = (unsigned char) CHUNK_RESPONSE;
  _size++;

  // Set path ID
  _path_id = path_id;
  _size++;

  // Set sender window size
  _size++; // Sender window field size
  _sender_wnd = sender_wnd;
  _size += std::to_string(_sender_wnd).size();

  // Set payload
  _payload = new char[payload_len];
  memcpy(_payload, payload, payload_len);
  _payload_len = payload_len;

  _size += _payload_len;
}

size_t
ChunkResponse::toBytes(char* data)
{
  size_t pos = 0;

  // Encode chunk type
  data[pos++] = _type;

  // Encode path ID
  data[pos++] = _path_id;

  // Encode sender window field size
  std::string sender_wnd_str = std::to_string(_sender_wnd);
  data[pos++] = sender_wnd_str.size();

  // Encode sender window field size
  memcpy(data + pos, sender_wnd_str.c_str(), sender_wnd_str.size());
  pos += sender_wnd_str.size();

  // Encode payload
  memcpy(data + pos, _payload, _payload_len);
  pos += _payload_len;

  return pos;
}

