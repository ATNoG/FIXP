/** Brief: HTTP to FP7 PURSUIT converter plugin
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

#include "pursuit-to-http.hpp"

extern "C" PursuitToHttpConverter* create_plugin_object()
{
  return new PursuitToHttpConverter();
}

extern "C" void destroy_object(PursuitToHttpConverter* object)
{
  delete object;
}

std::vector<std::string>
PursuitToHttpConverter::extractUrisFrom(MetaMessage& in)
{
  std::vector<std::string> ret;
  
  return ret;
}

std::string PursuitToHttpConverter::uriToAbsoluteForm(std::string uri, std::string parent)
{
  return "";
}

std::string
PursuitToHttpConverter::convertContent(MetaMessage& in,
                                       std::vector<std::string>& uris,
                                       std::map<std::string, std::string>& mappings)
{
  std::string content = "";

  return content;
}

