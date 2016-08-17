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

#include "plugin-pursuit-multipath.hpp"
#include "pursuit/chunk.hpp"

#include <fstream>

#define PURSUIT_ID_LEN_HEX_FORMAT 2 * PURSUIT_ID_LEN

extern "C" PursuitMultipathPlugin* create_plugin_object()
{
  return new PursuitMultipathPlugin;
}

extern "C" void destroy_object(PursuitMultipathPlugin* object)
{
  delete object;
}

int PursuitMultipathPlugin::subscribe_item(const Uri uri,
                                           unsigned char strategy)
{
  std::string uri_wo_schema = uri.toString().erase(0, strlen(SCHEMA) + 1);

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos,
                                               PURSUIT_ID_LEN_HEX_FORMAT);

  ba->subscribe_info(hex_to_chararray(id),
                     hex_to_chararray(prefix_id),
                     strategy,
                     NULL,
                     0);

  return 0;
}

int PursuitMultipathPlugin::unsubscribe_item(const Uri uri,
                                             unsigned char strategy)
{
  std::string uri_wo_schema = uri.toString().erase(0, strlen(SCHEMA) + 1);

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos,
                                               PURSUIT_ID_LEN_HEX_FORMAT);

  ba->unsubscribe_info(hex_to_chararray(id),
                       hex_to_chararray(prefix_id),
                       strategy,
                       NULL,
                       0);

  return 0;
}

int PursuitMultipathPlugin::subscribe_scope(const Uri uri,
                                            unsigned char strategy)
{
  std::string uri_wo_schema = uri.toString().erase(0, strlen(SCHEMA) + 1);

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos,
                                               PURSUIT_ID_LEN_HEX_FORMAT);

  ba->subscribe_scope(hex_to_chararray(id),
                      hex_to_chararray(prefix_id),
                      strategy,
                      NULL,
                      0);

  return 0;
}

int PursuitMultipathPlugin::unsubscribe_scope(const Uri uri,
                                              unsigned char strategy)
{
  std::string uri_wo_schema = uri.toString().erase(0, strlen(SCHEMA) + 1);

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos,
                                               PURSUIT_ID_LEN_HEX_FORMAT);

  ba->unsubscribe_scope(hex_to_chararray(id),
                        hex_to_chararray(prefix_id),
                        strategy,
                        NULL,
                        0);

  return 0;
}

int PursuitMultipathPlugin::publish_data(const Uri uri,
                                         unsigned char strategy,
                                         unsigned char* fid,
                                         void* content,
                                         size_t content_size)
{
  std::string uri_wo_schema = uri.toString().erase(0, strlen(SCHEMA) + 1);

  ba->publish_data(hex_to_chararray(uri_wo_schema),
                   strategy,
                   fid,
                   (fid == NULL ? 0 : FID_LEN),
                   content,
                   content_size);

  return 0;
}

void PursuitMultipathPlugin::processUri(const Uri uri)
{
  ba = Blackadder::Instance(true);

  subscribe_scope(uri, IMPLICIT_RENDEZVOUS);
  subscribe_item(uri.toString().append("ffffffffffffffff"), MULTIPATH);

  size_t chunk_no = 0;
  unsigned char f_bytearray[FID_LEN];
  unsigned char rf_bytearray[FID_LEN];

  bool is_msg_received = false;
  while (!is_msg_received) {
    Event ev;
    ba->getEvent(ev);
    switch (ev.type) {
      case START_PUBLISH: {
        std::string fid = ev.FIDs.substr(0, FID_LEN * 8);
        std::string rfid = ev.FIDs.substr(FID_LEN * 8, FID_LEN * 8);

        // Convert FID and Reverse FID
        // from bit-array to byte-array (little-endian format)
        for(int i = FID_LEN - 1; i >= 0; --i) {
          int rf_byte = 0;
          int f_byte = 0;
          for(int j = (FID_LEN - i - 1) * 8; j < ((FID_LEN - i) * 8); ++j) {
            rf_byte = (rf_byte << 1) | (rfid.at(j) == '1' ? 1 : 0);
            f_byte = (f_byte << 1) | (fid.at(j) == '1' ? 1 : 0);
          }

          rf_bytearray[i] = rf_byte;
          f_bytearray[i] = f_byte;
        }

        ChunkRequest req((const char*) rf_bytearray, (char) 0);
        char reqBytes[req.size()];
        req.toBytes(reqBytes);

        // Convert chunk number to request into hex format
        std::stringstream chunk_no_hex;
        chunk_no_hex << std::setfill('0') << std::setw(16) << std::hex << chunk_no;

        publish_data(uri.toString().append(chunk_no_hex.str()),
                     IMPLICIT_RENDEZVOUS,
                     f_bytearray,
                     (void*) reqBytes,
                     req.size());
      } break;

      case PUBLISHED_DATA: {
          unsigned char type = ((char *)ev.data)[0];
          if(type == CHUNK_RESPONSE) {
            ChunkResponse resp((char*) ev.data, ev.data_len);
            size_t n = fwrite(resp.getPayload(), sizeof(char), resp.getPayloadLen(), stdout);
            if(resp.getPayloadLen() != n) {
              std::cerr << "Error while writing to stdout. ";
            }

            if(resp.getPayloadLen() < BUFSIZE) {
              is_msg_received = true;
            } else {
              ChunkRequest req((const char*) rf_bytearray, (char) 0);
              char reqBytes[req.size()];
              req.toBytes(reqBytes);

              std::stringstream chunk_no_ss;
              chunk_no_ss << std::setfill('0') << std::setw(16) << std::hex << ++chunk_no;

              publish_data(uri.toString().append(chunk_no_ss.str()),
                           IMPLICIT_RENDEZVOUS,
                           f_bytearray,
                           (void*) reqBytes,
                           req.size());
            }
          }

      } break;
    }
  }

  unsubscribe_scope(uri, IMPLICIT_RENDEZVOUS);
  unsubscribe_item(uri.toString().append("ffffffffffffffff"), MULTIPATH);

  ba->disconnect();
  delete ba;
}
