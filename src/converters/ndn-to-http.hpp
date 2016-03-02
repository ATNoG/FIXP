/** Brief: NDN to HTTP converter plugin
 *  Copyright (C) 2016
 *  Jose Quevedo <quevedo@av.it.pt>
 *  Carlos Guimaraes <cguimaraes@av.it.pt>
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

#ifndef NDN_TO_HTTP_CONVERTER__HPP_
#define NDN_TO_HTTP_CONVERTER__HPP_

#include "../plugin-converter.hpp"

#define HTTP_SCHEMA "http://"
#define NDN_SCHEMA "ndn://"

class NdnToHttpConverter : public PluginConverter
{
public:
  NdnToHttpConverter() { };
  ~NdnToHttpConverter() { };

  std::string getProtocolConvertion() { return "ndn-to-http"; };
  std::vector<std::string> extractUrisFrom(MetaMessage& in);
  std::string uriToAbsoluteForm(std::string uri, std::string parent);
  std::string convertContent(MetaMessage& in, std::vector<std::string>& uris, std::map<std::string, std::string>& mappings);
};

#endif /* NDN_TO_HTTP_CONVERTER__HPP_ */
