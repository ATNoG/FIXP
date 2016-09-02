/** Brief: Logger
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

#ifndef LOGGER__HPP_
#define LOGGER__HPP_

#include <mutex>
#include <iostream>
#include <chrono>

enum Level {
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR = 1 ,
  LOG_LEVEL_WARN  = 2,
  LOG_LEVEL_INFO  = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_TRACE = 5,
  LOG_LEVEL_ALL   = 255
};

class Logger
{
private:
  Level _level = LOG_LEVEL_INFO; // Default level: INFO
  std::mutex _mutex;

public:
  static Logger& getInstance()
  {
    static Logger instance;
    return instance;
  }

  Level getLevel() const
  {
    return _level;
  }

  void setLevel(const Level level)
  {
    _level = level;
  }

  void log(const std::string level, const std::string msg)
  {
    std::lock_guard<std::mutex> lock(_mutex);

    using namespace std::chrono;
    microseconds ms = duration_cast<microseconds>(system_clock::now().time_since_epoch());

    std::clog << ms.count() << " [" << level << "] " << msg << std::endl << std::flush;
  }

  Logger(Logger const&) = delete;
  void operator=(Logger const&) = delete;

private:
  Logger() {};
};

#define FIFU_LOG(level, type, msg)                 \
  if(Logger::getInstance().getLevel() >= level) {  \
    Logger::getInstance().log(#type, msg);         \
  }

#define FIFU_LOG_FATAL(msg) FIFU_LOG(LOG_LEVEL_FATAL, FATAL, msg)
#define FIFU_LOG_ERROR(msg) FIFU_LOG(LOG_LEVEL_ERROR, ERROR, msg)
#define FIFU_LOG_WARN(msg)  FIFU_LOG(LOG_LEVEL_WARN,  WARN,  msg)
#define FIFU_LOG_INFO(msg)  FIFU_LOG(LOG_LEVEL_INFO,  INFO,  msg)
#define FIFU_LOG_DEBUG(msg) FIFU_LOG(LOG_LEVEL_DEBUG, DEBUG, msg)
#define FIFU_LOG_TRACE(msg) FIFU_LOG(LOG_LEVEL_TRACE, TRACE, msg)

#endif /* LOGGER__HPP_ */

