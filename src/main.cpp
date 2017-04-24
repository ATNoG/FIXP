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

#include <algorithm>
#include <argp.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <signal.h>

#define USAGE "Usage: fixp [OPTIONS] -r <resource file> " \
              "-p <path_to_protocols> -c <path_to_converters> "

Core* core;

void signalHandler(int signum)
{
    // Stop Core execution
    core->stop();
}

void loadProtocols(Core& core, const std::string path)
{
  DIR *dir = opendir(path.c_str());
  if(dir == NULL) {
    FIFU_LOG_WARN("(Main) Error while opening protocol plugins folder");
    return;
  }

  struct dirent* dirEntry;
  while(dirEntry = readdir(dir)) {
    if(dirEntry->d_type == DT_REG) {
      core.loadProtocol(path + "/" + dirEntry->d_name);
    }
  }
}

void loadConverters(Core& core, const std::string path)
{
  DIR *dir = opendir(path.c_str());
  if(dir == NULL) {
    FIFU_LOG_WARN("(Main) Error while opening converter plugins folder");
    return;
  }

  struct dirent* dirEntry;
  while(dirEntry = readdir(dir)) {
    if(dirEntry->d_type == DT_REG) {
      core.loadConverter(path + "/" + dirEntry->d_name);
    }
  }
}

void loadLogger(const unsigned short level)
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

struct Options
{
  const char* path_to_resources;
  const char* path_to_protocols;
  const char* path_to_converters;
  int numWorkers;
  unsigned short verbosity;
  bool usage;
};

struct argp_option program_options[] = {
    {"resources",  'r', "PATH",  0,
       "Path to file containing the URI of known resources",       0},
    {"protocols",  'p', "PATH",  0,
       "Path to protocol endpoint plugins folder",                 0},
    {"converters", 'c', "PATH",  0,
       "Path to content endpoint plugins folder",                  0},
    {"workers",    'w', "VALUE", 0,
       "Number of conversions that can be handled simultaneously", 0},
    {"verbose",    'v', "VALUE", 0,
       "Produce verbose output",                                   0},
    {"usage",      -1,  "",      OPTION_HIDDEN | OPTION_ARG_OPTIONAL,
       "Print an usage example message", 0},
    {0}
  };

error_t parse_opt(int key, char* arg, struct argp_state *state)
{
  struct Options *options = (Options*) state->input;
  switch(key) {
    case -1: {
      options->usage = true;
    } break;

    case 'c': {
      options->path_to_converters = arg;
    } break;

    case 'p': {
      options->path_to_protocols = arg;
    } break;

    case 'r': {
      options->path_to_resources = arg;
    } break;

    case 'v': {
      options->verbosity = atoi(arg);
    } break;

    case 'w': {
      options->numWorkers = atoi(arg);
    } break;

    default: {
      return ARGP_ERR_UNKNOWN;
    }
  }

  return 0;
}

int parseCmdOptions(int& argc, char**& argv,
                    const char*& path_to_resources,
                    const char*& path_to_protocols,
                    const char*& path_to_converters,
                    int& numWorkers, unsigned short& verbosity)
{
  struct Options options;

  // Default option values
  options.path_to_resources = NULL;
  options.path_to_protocols = NULL;
  options.path_to_converters = NULL;
  options.numWorkers = -1;
  options.verbosity = 3;
  options.usage = false;

  struct argp argp = { program_options, parse_opt, "", "OPTION:" };
  argp_parse(&argp, argc, argv, 0, 0, &options);

  if(options.usage) {
    std::cout << USAGE << std::endl << std::flush;
    return -1;
  }

  path_to_resources = options.path_to_resources;
  if(path_to_resources == NULL) {
    std::cout << "Missing mandatory argument ( -r, --resources=PATH )"
              << std::endl << std::flush;
    argp_help(&argp, stdout, ARGP_HELP_STD_ERR, "");
    return -1;
  }

  path_to_protocols = options.path_to_protocols;
  if(path_to_protocols == NULL) {
    std::cout << "Missing mandatory argument ( -p, --protocols=PATH )"
              << std::endl << std::flush;
    argp_help(&argp, stdout, ARGP_HELP_STD_ERR, "");
    return -1;
  }

  path_to_converters = options.path_to_converters;
  if(path_to_converters == NULL) {
    std::cout << "Missing mandatory argument ( -c, --converters=PATH )"
              << std::endl << std::flush;
    argp_help(&argp, stdout, ARGP_HELP_STD_ERR, "");
    return -1;
  }

  numWorkers = options.numWorkers;

  verbosity = options.verbosity;

  return 0;
}

int main(int argc, char* argv[])
{
  signal(SIGINT, signalHandler);

  const char* path_to_resources;
  const char* path_to_protocols;
  const char* path_to_converters;
  int numWorkers;
  unsigned short verbosity;

  int ret = parseCmdOptions(argc, argv,
                            path_to_resources, path_to_protocols,
                            path_to_converters, numWorkers, verbosity);

  if(ret != 0) {
    return ret;
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

