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

#include "plugin-pursuit.hpp"

#include <fstream>

#define PURSUIT_ID_LEN_HEX_FORMAT 2 * PURSUIT_ID_LEN
#define SCHEMA "pursuit://"

extern "C" PursuitPlugin* create_plugin_object()
{
  return new PursuitPlugin;
}

extern "C" void destroy_object(PursuitPlugin* object)
{
  delete object;
}

int PursuitPlugin::subscribe_item(std::string uri)
{
  std::string uri_wo_schema = uri.substr(std::string(SCHEMA).size());

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  ba->subscribe_info(hex_to_chararray(id),
                     hex_to_chararray(prefix_id),
                     DOMAIN_LOCAL,
                     NULL,
                     0);

  return 0;
}

int PursuitPlugin::unsubscribe_item(std::string uri)
{
  std::string uri_wo_schema = uri.substr(std::string(SCHEMA).size());

  size_t id_init_pos    = uri_wo_schema.size() - PURSUIT_ID_LEN_HEX_FORMAT;
  std::string prefix_id = uri_wo_schema.substr(0, id_init_pos);
  std::string id        = uri_wo_schema.substr(id_init_pos, PURSUIT_ID_LEN_HEX_FORMAT);

  ba->unsubscribe_info(hex_to_chararray(id),
                       hex_to_chararray(prefix_id),
                       DOMAIN_LOCAL,
                       NULL,
                       0);

  return 0;
}

void PursuitPlugin::processUri(std::string uri)
{
  ba = Blackadder::Instance(true);

  subscribe_item(uri);
  bool is_msg_received = false;
  while (!is_msg_received) {
    Event ev;
    ba->getEvent(ev);
    switch (ev.type) {
      case PUBLISHED_DATA:
        unsubscribe_item(uri);

        size_t n = fwrite(ev.data, sizeof(char), ev.data_len, stdout);
        if(ev.data_len != n) {
          std::cerr << "Error while writing to stdout. ";
        }

        is_msg_received = true;
      }
  }

  ba->disconnect();
  delete ba;
}
