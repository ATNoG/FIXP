/** Brief: Initial program execution point
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

#include "core.hpp"
#include "logger.hpp"
#include "thread-pool.hpp"

#include <iostream>
#include <fstream>
#include <signal.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#define USAGE "Usage: fixp [OPTIONS] -r <resource file> " \
              "-p <path_to_protocols> -c <path_to_converters> "

Core* core;

void signalHandler(int signum)
{
    // Stop Core execution
    core->stop();
}

void loadProtocols(Core& core, std::string path)
{
  boost::filesystem::path dir(path);
  if(!boost::filesystem::exists(dir) ||
     !boost::filesystem::is_directory(dir)) {
    return;
  }

  for(boost::filesystem::directory_iterator it(dir);
      it != boost::filesystem::directory_iterator();
      ++it) {
    if(boost::filesystem::is_regular_file(it->path())) {
      core.loadProtocol(it->path().string());
    }
  }
}

void loadConverters(Core& core, std::string path)
{
  boost::filesystem::path dir(path);
  if(!boost::filesystem::exists(dir) ||
     !boost::filesystem::is_directory(dir)) {
    return;
  }

  for(boost::filesystem::directory_iterator it(dir);
      it != boost::filesystem::directory_iterator();
      ++it) {
    if(boost::filesystem::is_regular_file(it->path())) {
      core.loadConverter(it->path().string());
    }
  }
}

void loadLogger(unsigned short level)
{
  Logger::getInstance().setLevel(static_cast<Level>(level));
}

void load_resources(Core& core, std::string path)
{
  std::ifstream file(path);

  std::string line;
  while(std::getline(file, line)) {
    // Trim white-spaces
    line.erase(std::remove_if(line.begin(),
                              line.end(),
                              [](char x){return std::isspace(x);}),
               line.end());

    // Ignore comments (everything after '#')
    if(line.front() == '#' || line.size() == 0) {
      continue;
    }

    core.createMapping(line.substr(0, line.find("#")));
  }

  file.close();
}

int main(int argc, char* argv[])
{
  signal(SIGINT, signalHandler);

  std::string path_to_resources;
  std::string path_to_protocols;
  std::string path_to_converters;
  int numWorkers;
  unsigned short verbosity;

  boost::program_options::options_description desc("Options");
  boost::program_options::variables_map vm;
  try {
    desc.add_options()
      ("resources,r", boost::program_options::value<std::string>(&path_to_resources)->required(),
                "Path to file containing known resource URIs")
      ("protocols,p", boost::program_options::value<std::string>(&path_to_protocols)->required(),
                "Path to protocol endpoint plugins")
      ("converters,c", boost::program_options::value<std::string>(&path_to_converters)->required(),
                "Path to protocol convertion plugins")
      ("workers,w", boost::program_options::value<int>(&numWorkers)->default_value(-1),
                "Number of conversions that can be handle simultaneously")
      ("verbose,v", boost::program_options::value<unsigned short>(&verbosity)->default_value(3),
                "Produce verbose output")
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
      std::cout << "Error: " << e.what() << std::endl << std::flush;
      return 1;
    }
  }

  loadLogger(verbosity);

  ThreadPool tp(numWorkers);
  core = new Core(tp);

  // Load plugins
  loadProtocols(*core, path_to_protocols);
  loadConverters(*core, path_to_converters);

  // Load known resource and create mappings on each architecture
  load_resources(*core, path_to_resources);

  core->start();

  delete core;
  return 0;
}

