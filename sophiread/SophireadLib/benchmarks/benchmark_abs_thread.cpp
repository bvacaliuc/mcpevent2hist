/**
 * @file benchmark_abs_pthread.cpp
 * @author Chen Zhang (zhangc@ornl.gov)
 * @brief benchmark abs clustering method with std::thread
 * @version 0.1
 * @date 2023-08-25
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "abs.h"

using namespace std;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> pos(0, 2);
std::uniform_real_distribution<> tot(0, 100);
std::uniform_real_distribution<> toa(0, 1000);
std::uniform_real_distribution<> ftoa(0, 255);
std::uniform_real_distribution<> tof(0, 2000);
std::uniform_real_distribution<> spidertime(-1, 1);

/*
Target processing speed: 120,000,000 hits / sec -> 120 hits/us
  -> 12 clusters, 10 hits each == 120 hits -> 1 us
  -> 12000 clusters, 10 hits each == 120000 hits -> 1000 us
*/

std::vector<Hit> fake_hits() {
  std::vector<Hit> hits;
  const int num_clusters = 12000000;
  const int num_hits_per_cluster = 10;
  hits.reserve(num_clusters * num_hits_per_cluster);

  // generate
  for (int i = 0; i < num_clusters; i++) {
    // cluster center
    int x = 10 * i + pos(gen);
    int y = 10 * i + pos(gen);
    int stime = 10 * i + spidertime(gen);
    // cluster
    for (int j = 0; j < num_hits_per_cluster; j++) {
      hits.emplace_back(
          Hit(x, y, tot(gen), toa(gen), ftoa(gen), tof(gen), stime));
    }
  }
  return hits;
}

struct thread_data {
  std::vector<Hit>::const_iterator begin;
  std::vector<Hit>::const_iterator end;

  void run() {
    ABS abs_alg(5.0, 1, 75);
    // fit hits into clusters
    abs_alg.fit(std::vector<Hit>(begin, end));

    // get neutron events
    abs_alg.get_events(std::vector<Hit>(begin, end));
  }
};

double single_test(const std::vector<Hit>& hits, int num_thread) {
  // record time
  auto start = std::chrono::high_resolution_clock::now();

  // chunk size
  size_t chunk_size = hits.size() / num_thread;

  std::vector<thread_data> thread_data_list(num_thread);
  std::vector<std::thread> threads(num_thread);

  // start threads
  for (int i = 0; i < num_thread; ++i) {
    thread_data_list[i].begin = hits.begin() + i * chunk_size;
    thread_data_list[i].end = (i == num_thread - 1)
                                  ? hits.end()
                                  : hits.begin() + (i + 1) * chunk_size;
    threads[i] = std::thread(&thread_data::run, std::ref(thread_data_list[i]));
  }

  // join threads
  for (int i = 0; i < num_thread; ++i) {
    threads[i].join();
  }

  // record time
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count() /
      1e6;
  cout << "[user]total " << duration << " sec" << endl;

  return duration;
}

int main() {
  // create fake hits
  auto hits = fake_hits();
  size_t num_threads = 64;

  // run 100 tests and get the average
  const int num_tests = 1000;
  double total_time = 0;
  for (int i = 0; i < num_tests; i++) {
    total_time += single_test(hits, num_threads);
  }
  cout << "average " << total_time / num_tests << " sec" << endl;
  return 0;
}
