/** Brief: Get a resource matching an URI and write it to stdout
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

#include "plugin-manager.hpp"

#include <iostream>
#include <boost/program_options.hpp>

#define USAGE "ficatresource [OPTIONS] --uri <uri>"

int main (int argc, char* argv[])
{
  std::string uri;
  std::string path_to_plugins;

  boost::program_options::options_description desc("Options");
  boost::program_options::variables_map vm;
  try {
    desc.add_options()
      ("uri,u", boost::program_options::value<std::string>(&uri)->required(),
                "Resource URI")
      ("plugins,p", boost::program_options::value<std::string>(&path_to_plugins)->required(),
                "Path to protocol plugins")
      ("help,h", "Display configuration options");

    boost::program_options::store(boost::program_options::parse_command_line(argc,
                                                                             argv,
                                                                             desc),
                                  vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
      std::cout << USAGE << std::endl << desc << std::endl << std::flush;
      return 0;
    }
  } catch(std::exception& e) {
    if (vm.count("help")) {
      std::cout << USAGE << std::endl << desc << std::endl << std::flush;
      return 0;
    } else {
      std::cout << USAGE << std::endl << desc << std::endl;
      std::cerr << "Error: " << e.what() << std::endl << std::flush;
      return 1;
    }
  }

  PluginManager pm;
  pm.loadPlugins(path_to_plugins);

  pm.forwardUriToPlugin(uri);

  return 0;
}

