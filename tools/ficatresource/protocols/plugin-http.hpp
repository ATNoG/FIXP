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

#ifndef HTTP_PLUGIN__HPP_
#define HTTP_PLUGIN__HPP_

#include "../plugin.hpp"

class HttpPlugin : public Plugin
{
public:
  HttpPlugin()
    : Plugin()
  { }

  std::string getSchema() const { return "http"; }
  void processUri(const std::string uri);
};

#endif /* HTTP_PLUGIN__HPP_ */

