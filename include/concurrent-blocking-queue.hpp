/** Brief: Concurrent Blocking Queue
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

#ifndef CONCURRENT_BLOCKING_QUEUE__HPP_
#define CONCURRENT_BLOCKING_QUEUE__HPP_

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ConcurrentBlockingQueue
{
private:
  bool _interrupted = false;

  std::queue<T> _queue;
  mutable std::mutex _mutex;
  std::condition_variable _notifier;

public:
  ~ConcurrentBlockingQueue()
  { }

  void stop()
  {
    _interrupted = true;
    _notifier.notify_all();
  }

  bool empty() const
  {
    std::unique_lock<std::mutex> lock(_mutex);
    return _queue.empty();
  }

  size_t size() const
  {
    std::unique_lock<std::mutex> lock(_mutex);
    return _queue.size();
  }

  void push(const T& v)
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _queue.push(v);

    lock.unlock();
    _notifier.notify_one();
  }

  T& pop()
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _notifier.wait(lock, [this](){return !_queue.empty() || _interrupted;});
    if(_interrupted) {
      throw std::exception();
    }

    T& v = _queue.front();
    _queue.pop();

    return v;
  }

};

#endif /* CONCURRENT_BLOCKING_QUEUE__HPP_ */

