/** Brief: Plugin Converter Factory
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

#ifndef PLUGIN_CONVERTER_FACTORY__HPP_
#define PLUGIN_CONVERTER_FACTORY__HPP_

#include "plugin-converter.hpp"

#include <dlfcn.h>
#include <memory>

class PluginConverterFactory {
public:
  static const std::shared_ptr<PluginConverter> createPlugin(const std::string path)
  {
    void* handle = dlopen(path.c_str(), RTLD_LAZY);

    PluginConverter* (*create)()
      = (PluginConverter* (*)()) dlsym(handle, "create_plugin_object");
    PluginConverter* plugin = create();

    return std::shared_ptr<PluginConverter>(plugin);
  }
};

#endif /* PLUGIN_CONVERTER_FACTORY__HPP_ */

