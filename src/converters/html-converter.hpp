/** Brief: HTML converter plugin
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

#ifndef HTML_CONVERTER__HPP_
#define HTML_CONVERTER__HPP_

#include "../plugin-converter.hpp"

class HtmlConverter : public PluginConverter
{
public:
  HtmlConverter() { };
  ~HtmlConverter() { };

  std::vector<std::string> getFileTypes() const { return {"text/html"}; };
  std::map<std::string, Uri> extractUrisFromContent(const Uri uri, const std::string content);
  std::string convertContent(const std::string content, const std::map<std::string, Uri>& mappings);
};

#endif /* HTML_CONVERTER__HPP_ */

