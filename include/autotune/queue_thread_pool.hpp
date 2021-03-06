#pragma once

#include "execution_wrapper.hpp"
#include "opttmp/bits.hpp"
#include "opttmp/numa_topology.hpp"
#include "thread_safe_queue.hpp"
// #include <boost/thread.hpp>
#include <condition_variable>
#include <errno.h>
#include <iostream>
#include <pthread.h>
#include <thread>

// extern std::mutex print_mutex;

// thread_local size_t thread_id;

// size_t get_thread_id() { return thread_id; }

namespace autotune {

enum class affinity_type_t : uint64_t { full = 0, compact = 1, sparse = 2 };

template <size_t num_threads> class queue_thread_pool {
private:
  std::array<std::thread, num_threads> threads;
  std::condition_variable threads_wait_cv;
  // used for threads to wait for work. Also used to lock for finishing
  std::mutex mutex_threads;
  bool threads_finish;

  detail::thread_safe_queue<std::unique_ptr<detail::abstract_executor>> safe_q;

  // uint32_t physical_concurrency;

  bool verbose;

  opttmp::numa_topology_t numa_topology;
  // cpu_set_t affinity_set;
  std::vector<bool> affinity_set;

  static void delete_cpu_set(cpu_set_t *cpu_set) { CPU_FREE(cpu_set); }

  void worker_main(size_t i) {

    // set thread affinity to the core of your number
    pthread_t thread = pthread_self();
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // CPU_SET(static_cast<int>(i % physical_concurrency), &cpuset);
    std::shared_ptr<cpu_set_t> cpu_set(
        CPU_ALLOC(numa_topology.get_threads_total()), delete_cpu_set);
    size_t cpu_set_size_bytes =
        CPU_ALLOC_SIZE(numa_topology.get_threads_total());
    CPU_ZERO_S(cpu_set_size_bytes, cpu_set.get());
    int64_t thread_cpu = 0;
    uint64_t counter = 0;
    for (size_t j = 0; j < numa_topology.get_threads_total(); j += 1) {
      if (affinity_set[j]) {
        if (counter == i) {
          thread_cpu = j;
          break;
        }
        counter += 1;
      }
    }
    CPU_SET(thread_cpu, cpu_set);
    // numa_topology.print_cpu_set(cpu_set);
    if (verbose) {
      std::cout << "queue_thread_pool: thread i: " << i
                << " bound to core: " << thread_cpu << std::endl;
    }

    int s = pthread_setaffinity_np(thread, cpu_set_size_bytes, cpu_set.get());
    if (s != 0) {
      errno = s;
      perror("pthread_setaffinity_np");
      exit(EXIT_FAILURE);
    }

    while (true) {

      // check for work
      std::unique_ptr<detail::abstract_executor> exe;
      if (safe_q.reserve()) {
        // work found
        exe = std::move(safe_q.next());
      }

      if (exe) {
        exe->set_thread_id(i);

        // if work was found, execute it
        // if (auto *casted =
        //         dynamic_cast<detail::delayed_executor_id *>(exe.get())) {
        //   (*casted)(i);
        // } else {
        (*exe)();
        // }
      } else {
        // no work was found, wait for work or finish signal
        std::unique_lock lock(mutex_threads);
        // spurious wakeups can occur
        // this is not a problem a thread will re-check for work and then
        // continue sleeping
        if (threads_finish) {
          // finish signal received
          break;
        }
        threads_wait_cv.wait(lock); // , [this]() { return threads_finish; }
      }
    }
  }

public:
  int64_t THREAD_ID_PLACEHOLDER = 0;

  queue_thread_pool(bool verbose = false)
      : threads_finish(false),
        // physical_concurrency(boost::thread::physical_concurrency()),
        verbose(verbose) {
    // std::cout << "using no. of cpu cores: " << cpu_cores << std::endl;
    // std::cout << "boost physical concurrency: "
    //           << boost::thread::physical_concurrency() << std::endl;
    // std::cout << "boost hardware concurrency: "
    //           << boost::thread::hardware_concurrency() << std::endl;
    // if (verbose) {
    //   std::cout << "queue_thread_pool: physical_concurrency: "
    //             << physical_concurrency << std::endl;
    // }
    if (num_threads > numa_topology.get_cores_total()) {
      std::cout << "queue_thread_pool: warning: using more threads than "
                   "available cores: "
                << num_threads << " threads > "
                << numa_topology.get_cores_total() << " cores" << std::endl;
    }
    affinity_set = numa_topology.get_full();
  } // next_work(0), last_work(0)

  // creates and starts threads of thread pool (non-blocking)
  void start() {
    // TODO: how to the reset the class? cv variable? host_finish?
    for (size_t i = 0; i < num_threads; i++) {
      threads[i] =
          std::thread(&queue_thread_pool<num_threads>::worker_main, this, i);
    }
  }

  // trigger shutting down the thread pool (blocking), waits for all remaining
  // work to be processed
  void finish() {
    // lock to prevent racecondition between "threads_finish" and subsequent
    // wait
    mutex_threads.lock();
    threads_finish = true;
    threads_wait_cv.notify_all();
    // need to unlock for the join to work
    mutex_threads.unlock();
    for (size_t i = 0; i < num_threads; i++) {
      threads[i].join();
    }
  }

  // enqueue arbitrary function with void return, arguments have to be copyable
  template <typename... Args>
  void enqueue_work(std::function<void(Args...)> f, Args... args) {
    auto work_temp = std::unique_ptr<detail::abstract_executor>(
        new detail::delayed_executor<Args...>(f, std::move(args)...));
    safe_q.push_back(std::move(work_temp));
    threads_wait_cv.notify_one();
  }

  template <typename F, typename... Args> void enqueue_work(F f, Args... args) {
    auto work_temp = std::unique_ptr<detail::abstract_executor>(
        new detail::delayed_executor<Args...>(f, std::move(args)...));
    safe_q.push_back(std::move(work_temp));
    threads_wait_cv.notify_one();
  }

  // enqueue arbitrary function with void return, arguments have to be copyable
  template <typename... Args>
  void enqueue_work_id(std::function<void(Args...)> f, Args... args) {
    auto work_temp = std::unique_ptr<detail::abstract_executor>(
        new detail::delayed_executor_id<Args...>(f, std::move(args)...));
    safe_q.push_back(std::move(work_temp));
    threads_wait_cv.notify_one();
  }

  // has to be called before start
  void set_affinity(affinity_type_t affinity_type) {
    if (affinity_type == affinity_type_t::full) {
      affinity_set = numa_topology.get_full();
    } else if (affinity_type == affinity_type_t::compact) {
      affinity_set = numa_topology.get_compact(num_threads);
    } else { // (affinity_set == affinity_type_t::sparse)
      affinity_set = numa_topology.get_sparse(num_threads);
    }
  }

  void set_custom_affinity(std::array<uint32_t, num_threads> cpu_indices) {
    affinity_set = numa_topology.get_empty();
    for (uint32_t v : cpu_indices) {
      affinity_set[v] = true;
    }
  }

  // // // enqueue function with void return that accepts thread metadata as its
  // first
  // // // argument. Arguments have to be copyable
  // // template <typename... Args>
  // // void enqueue_work(std::function<void(thread_meta, Args...)> f, Args...
  // // args) {
  // //   auto work_temp = std::unique_ptr<detail::abstract_executor>(
  // //       new detail::delayed_executor_meta<Args...>(f, args...));
  // //   // print_mutex.lock();
  // //   // std::cout << "enqueue work with meta (holding mutex)" << std::endl;
  // //   // print_mutex.unlock();
  // //   safe_q.push_back(std::move(work_temp));
  // //   threads_wait_cv.notify_one();
  // //   // print_mutex.lock();
  // //   // std::cout << "ONE thread notified" << std::endl;
  // //   // std::cout << "enqueue work with meta (releasing mutex)" <<
  // std::endl;
  // //   // print_mutex.unlock();
  // // }
};
} // namespace autotune
