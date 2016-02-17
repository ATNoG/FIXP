/** Brief: Plugin Factory
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

#ifndef PLUGIN_FACTORY__HPP_
#define PLUGIN_FACTORY__HPP_

#include "plugin.hpp"

#include <dlfcn.h>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

class PluginFactory {
public:

  static boost::shared_ptr<Plugin> createPlugin(std::string path_to_plugin)
  {
    void* handle = dlopen(path_to_plugin.c_str(), RTLD_LAZY);

    Plugin* (*create)();
    create = (Plugin* (*)())dlsym(handle, "create_plugin_object");
    Plugin* teste = (Plugin*) create();

    return boost::shared_ptr<Plugin>(teste);
  }
};

#endif /* PLUGIN_FACTORY__HPP_ */

