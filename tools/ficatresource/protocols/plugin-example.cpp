/** Brief: Example plugin implementation
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

#include "plugin-example.hpp"

#include <iostream>

extern "C" ExamplePlugin* create_plugin_object()
{
  return new ExamplePlugin;
}

extern "C" void destroy_object(ExamplePlugin* object)
{
  delete object;
}

void ExamplePlugin::processUri(const std::string uri)
{
  std::cout << "ExamplePlugin requesting " << uri << std::endl << std::flush;
}

