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

#ifndef FP7_PURSUIT_PLUGIN__HPP_
#define FP7_PURSUIT_PLUGIN__HPP_

#include "../plugin.hpp"

#include <blackadder.hpp>

#define SCHEMA "pursuit"

class PursuitPlugin : public Plugin
{
public:
  PursuitPlugin()
    : Plugin()
  { }

  std::string getSchema() const { return SCHEMA; }
  void processUri(const Uri uri);

private:
  int subscribe_item(const Uri uri);
  int unsubscribe_item(const Uri uri);

  Blackadder *ba;
};

#endif /* FP7_PURSUIT_PLUGIN__HPP_ */

