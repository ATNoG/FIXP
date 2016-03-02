/** Brief: NDN plugin implementation
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

#include "plugin-ndn.hpp"

#include <iostream>

extern "C" NdnPlugin* create_plugin_object()
{
  return new NdnPlugin;
}

extern "C" void destroy_object(NdnPlugin* object)
{
  delete object;
}

std::string removeSchemaFromUri(std::string uri)
{
  std::string schema_division = "://";
  size_t pos = uri.find(schema_division); //TODO: Schema division not found
  return uri.substr(pos+schema_division.length());
}

void NdnPlugin::onData(const Interest& interest, const Data& data)
{
  std::cout << data << std::endl;
  Block content = data.getContent();
  std::cout << std::string(reinterpret_cast<const char*>(content.value()),
                           content.value_size())
  << std::endl;
}

void NdnPlugin::onTimeout(const Interest& interest)
{
  std::cout << "Timeout" << interest << std::endl;
}

void NdnPlugin::processUri(std::string uri)
{
  std::cout << "NdnPlugin requesting " << uri << std::endl << std::flush;
  Interest interest(Name(removeSchemaFromUri(uri)));
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setMustBeFresh(true);

  _face.expressInterest(interest,
                         bind(&NdnPlugin::onData, this,  _1, _2),
                         bind(&NdnPlugin::onTimeout, this, _1));

  std::cout << "Sending " << interest << std::endl;

  // processEvents will block until the requested data received or timeout occurs
  _face.processEvents();
}

