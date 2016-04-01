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

#include <argp.h>
#include <iostream>

#define USAGE "ficatresource -p <path_to_protocols> -uri <uri_to_request>"

struct Options
{
  const char* path_to_protocols;
  const char* uri_to_request;
  bool usage;
};

struct argp_option program_options[] = {
    {"plugins",  'p', "PATH", 0,
       "Path to protocol endpoint plugins folder", 0},
    {"uri",      'u', "URI",  0,
       "URI of the resource to request",           0},
    {"usage",      -1,  "",   OPTION_HIDDEN | OPTION_ARG_OPTIONAL,
       "Print an usage example message",           0},
    {0}
  };

error_t parse_opt(int key, char* arg, struct argp_state *state)
{
  struct Options *options = (Options*) state->input;
  switch(key) {
    case -1: {
      options->usage = true;
    } break;

    case 'p': {
      options->path_to_protocols = arg;
    } break;

    case 'u': {
      options->uri_to_request = arg;
    } break;

    default: {
      return ARGP_ERR_UNKNOWN;
    }
  }

  return 0;
}

int parseCmdOptions(int& argc, char**& argv,
                    const char*& path_to_protocols,
                    const char*& uri_to_request)
{
  struct Options options;

  // Default option values
  options.path_to_protocols = NULL;
  options.uri_to_request = NULL;
  options.usage = false;

  struct argp argp = { program_options, parse_opt, "", "OPTION:" };
  argp_parse(&argp, argc, argv, 0, 0, &options);

  if(options.usage) {
    std::cout << USAGE << std::endl << std::flush;
    return -1;
  }

  path_to_protocols = options.path_to_protocols;
  if(path_to_protocols == NULL) {
    std::cout << "Missing mandatory argument ( -p, --protocols=PATH )"
              << std::endl << std::flush;
    argp_help(&argp, stdout, ARGP_HELP_STD_ERR, "");
    return -1;
  }

  uri_to_request = options.uri_to_request;
  if(uri_to_request == NULL) {
    std::cout << "Missing mandatory argument ( -u, --uri=URI )"
              << std::endl << std::flush;
    argp_help(&argp, stdout, ARGP_HELP_STD_ERR, "");
    return -1;
  }

  return 0;
}

int main (int argc, char* argv[])
{
  const char* path_to_protocols;
  const char* uri_to_request;

  int ret = parseCmdOptions(argc, argv,
                            path_to_protocols, uri_to_request);

  if(ret != 0) {
    return ret;
  }

  PluginManager pm;
  pm.loadPlugins(path_to_protocols);

  pm.forwardUriToPlugin(uri_to_request);

  return 0;
}

