/** Brief: Plugin Converter Interface
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

#ifndef PLUGIN_CONVERTER__HPP_
#define PLUGIN_CONVERTER__HPP_

#include "metamessage.hpp"

#include <map>
#include <vector>

class PluginConverter
{
public:
  PluginConverter() { };
  ~PluginConverter() { };

  virtual std::string getFileType() = 0;
  virtual std::map<std::string, std::string> extractUrisFromContent(std::string uri, std::string content) = 0;
  virtual std::string uriToAbsoluteForm(std::string uri, std::string parent) = 0;
  virtual std::string convertContent(std::string content, std::map<std::string, std::string>& mappings) = 0;
};

#endif /* PLUGIN_CONVERTER__HPP_ */

