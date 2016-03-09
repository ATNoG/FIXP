/** Brief: Thread Pool
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

#ifndef THREAD_POOL__HPP_
#define THREAD_POOL__HPP_

#include "concurrent-blocking-queue.hpp"

#include <atomic>
#include <functional>
#include <thread>

// Usage example:
// '''
//  (...)
//
//  ThreadPool tp(2);
//
//  // Using std::bind
//  std::function<void()> a(std::bind(&function, object, args));
//  tp.schedule(std::move(a));
//
//  // or using lambda functions
//  tp.schedule([](){std::cout << "Executing job" << std::endl << std::flush;});
//
//  (...)
// '''
//
class ThreadPool
{
private:
  std::atomic<bool> _isRunning;
  std::vector<std::thread> _workers;
  ConcurrentBlockingQueue<std::function<void()>*> _job_queue;

public:
  ThreadPool(int numWorkers = 0)
    : _isRunning(true)
  {
    if(numWorkers < 1) {
      numWorkers = std::thread::hardware_concurrency();
    }

    for(int i = 0; i < numWorkers; ++i) {
      _workers.push_back(std::thread(&ThreadPool::doSomething, this));
    }
  }

  ~ThreadPool()
  {
    _isRunning = false;
    _job_queue.stop();

    clear();
  }

  void schedule(const std::function<void()>&& function)
  {
    _job_queue.push(new std::function<void()>(std::move(function)));
  }

private:
  void doSomething()
  {
    std::function<void()>* job;

    while(_isRunning) {
      try {
        // Get next job on the queue
        job = _job_queue.pop();
      } catch(...) {
        return;
      }

      // Execute pending job
      (*job)();
      delete job;
    }
  }

  void clear()
  {
    joinWorkers();
    _workers.clear();
  }

  void joinWorkers()
  {
    for(auto& item : _workers) {
      item.join();
    }
  }

};

#endif /* THREAD_POOL__HPP_ */
