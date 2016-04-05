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

std::string removeSchemaFromUri(const std::string uri)
{
  std::string schema_division = "://";
  size_t pos = uri.find(schema_division); //TODO: Schema division not found
  return uri.substr(pos+schema_division.length());
}

void NdnPlugin::onData(const Interest& interest, const Data& data)
{
  //Check whether it is a chunk or a complete data
  if (data.getName()[-1].isSegment())
    onChunk(interest,data);
  else
  {
    std::cout << data << std::endl;
    Block content = data.getContent();
    std::cout << std::string(reinterpret_cast<const char*>(content.value()),
                             content.value_size())
    << std::endl;
  }
}

void NdnPlugin::requestChunk(const Name& interest_name)
{
  Interest interest(interest_name);
  interest.setInterestLifetime(time::milliseconds(5000));
  interest.setMustBeFresh(true);

  _face.expressInterest(interest,
                        bind(&NdnPlugin::onChunk, this,  _1, _2),
                        bind(&NdnPlugin::onChunkTimeout, this, _1));

}

void NdnPlugin::onChunk(const Interest& interest, const Data& data)
{

  const Name & data_name = data.getName();
  uint64_t chunk_no = data_name[-1].toSegment();
  uint64_t last_chunk_no = data.getFinalBlockId().toSegment();
  std::cout << std::string(reinterpret_cast<const char *>(data.getContent().value()),
                                                       data.getContent().value_size());
  //TODO: For now lets assume FinalBlockId will be available in every chunk
  if(chunk_no!=last_chunk_no)
  {
    const Name next_chunk = data_name.getPrefix(data_name.size() - 1)
                                            .appendSegment(chunk_no + 1);
    // Schedule a new event
    _scheduler.scheduleEvent(time::milliseconds(0),
                              bind(&NdnPlugin::requestChunk, this, next_chunk));
  }
  else
    std::cout << std::endl;
}

void NdnPlugin::onTimeout(const Interest& interest)
{
  std::cout << "Timeout" << interest << std::endl;
}

void NdnPlugin::onChunkTimeout(const Interest& interest)
{
  std::cout << "Chunk request timeout " << interest << std::endl;
}

void NdnPlugin::processUri(const std::string uri)
{
  std::cout << "NdnPlugin requesting " << uri << std::endl << std::flush;
  Interest interest(Name(removeSchemaFromUri(uri)));
  interest.setInterestLifetime(time::milliseconds(5000));
  interest.setMustBeFresh(true);

  _face.expressInterest(interest,
                         bind(&NdnPlugin::onData, this,  _1, _2),
                         bind(&NdnPlugin::onTimeout, this, _1));

  std::cout << "Sending " << interest << std::endl;

  // processEvents will block until the requested data received or timeout occurs
  _io_service.run();
}
