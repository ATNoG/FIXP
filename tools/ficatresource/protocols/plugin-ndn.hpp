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

#ifndef NDN_PLUGIN__HPP_
#define NDN_PLUGIN__HPP_

#include "../plugin.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#define SCHEMA "ndn"

using namespace ndn;

static const uint32_t MAX_CHUNK_SIZE = MAX_NDN_PACKET_SIZE >> 1;

class NdnPlugin : public Plugin
{
private:
  boost::asio::io_service _io_service;
  Face _face;
  Scheduler _scheduler;

public:
  NdnPlugin()
    : Plugin()
    , _face(_io_service)
    , _scheduler(_io_service)
  { }

  std::string getSchema() const { return SCHEMA; }
  void processUri(const Uri uri);

private:
  void sendInterest(const Name& name);

  void onData(const Interest& interest, const Data& data);
  void onTimeout(const Interest& interest);
  void onSegment(const Interest& interest, const Data& data);
  void onSegmentOffset(const Interest& interest, const Data& data);
};

#endif /* NDN_PLUGIN__HPP_ */
