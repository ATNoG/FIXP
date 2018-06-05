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

void NdnPlugin::onData(const Interest& interest, const Data& data)
{
  //Check whether it is a segment, an offset or a complete data
  if(data.getName()[-1].isSegment()) {
    onSegment(interest, data);
  } else if(data.getName()[-1].isSegmentOffset()) {
    onSegmentOffset(interest, data);
  } else {
    std::string content(reinterpret_cast<const char*>(data.getContent().value()),
                        data.getContent().value_size());

    size_t n = fwrite(content.c_str(), sizeof(char), content.size(), stdout);
    if(content.size() != n) {
      std::cerr << "Error while writing to stdout. ";
    }
  }
}

void NdnPlugin::sendInterest(const Name& name)
{
  Interest interest(name);
  interest.setInterestLifetime(time::milliseconds(5000));
  interest.setMustBeFresh(true);

  _face.expressInterest(interest,
                        bind(&NdnPlugin::onData, this,  _1, _2),
                        bind(&NdnPlugin::onTimeout, this, _1));
}

void NdnPlugin::onSegment(const Interest& interest, const Data& data)
{
  std::string content(reinterpret_cast<const char*>(data.getContent().value()),
                      data.getContent().value_size());

  size_t n = fwrite(content.c_str(), sizeof(char), content.size(), stdout);
  if(content.size() != n) {
    std::cerr << "Error while writing to stdout. ";
  }

  if((data.getFinalBlockId().empty() && data.getContent().value_size() == MAX_CHUNK_SIZE)
     || (!data.getFinalBlockId().empty() &&
         data.getName()[-1].toSegment() < data.getFinalBlockId().toSegment())) {

    // Request next segment
    const Name next_segment_name = data.getName().getSuccessor();

    // Schedule a new event
    _scheduler.scheduleEvent(time::milliseconds(0),
                             bind(&NdnPlugin::sendInterest, this, next_segment_name));
  }
}

void NdnPlugin::onSegmentOffset(const Interest& interest, const Data& data)
{
  std::string content(reinterpret_cast<const char*>(data.getContent().value()),
                      data.getContent().value_size());

  size_t n = fwrite(content.c_str(), sizeof(char), content.size(), stdout);
  if(content.size() != n) {
    std::cerr << "Error while writing to stdout. ";
  }

  if((data.getFinalBlockId().empty() && data.getContent().value_size() == MAX_CHUNK_SIZE)
     || (!data.getFinalBlockId().empty() &&
         data.getName()[-1].toSegmentOffset() + data.getContent().value_size() < data.getFinalBlockId().toSegmentOffset())) {

    // Request next segment offset
    const Name next_offset_name = data.getName().getPrefix(data.getName().size() - 1)
                                   .appendSegmentOffset(data.getName()[-1].toSegmentOffset() + data.getContent().value_size());

    // Schedule a new event
    _scheduler.scheduleEvent(time::milliseconds(0),
                              bind(&NdnPlugin::sendInterest, this, next_offset_name));
  }
}

void NdnPlugin::onTimeout(const Interest& interest)
{
  std::cout << "Timeout" << interest << std::endl;
}

void NdnPlugin::processUri(const Uri uri)
{
  Name name(uri.toString());
  sendInterest(name);

  // processEvents will block until the requested data received or timeout occurs
  _io_service.run();
}
