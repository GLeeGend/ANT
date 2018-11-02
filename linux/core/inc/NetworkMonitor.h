/* Copyright 2017-2018 All Rights Reserved.
 *  Gyeonghwan Hong (redcarrottt@gmail.com)
 *
 * [Contact]
 *  Gyeonghwan Hong (redcarrottt@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0(the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __NETWORK_MONITOR_H__
#define __NETWORK_MONITOR_H__

#include "API.h"
#include "NetworkSwitcher.h"

namespace sc {
class Stats {
public:
  /* Statistics used to print present status */
  float ema_queue_arrival_speed = 0;

  /* Statistics used in CoolSpots Policy */
  int now_total_bandwidth = 0;

  /* Statistics used in Energy-aware & Latency-aware Policy */
  float ema_send_request_size = 0;
  float ema_arrival_time_us = 0;
  int now_queue_data_size = 0;

  /* Statistics used to evaluate the policies */
  float ema_send_rtt = 0;
};

class NetworkSwitcher;
class NetworkMonitor {
public:
  /* Control netwowrk monitor thread */
  void start(void);
  void stop(void);
  
  /* Control logging thread */
  void start_logging(void);
  void stop_logging(void);

private:
  /* Network monitor thread */
  void monitor_thread(void);
  std::thread *mMonitorThread;
  bool mMonitorThreadOn;

  /* Logging thread */
  void logging_thread(void);
  std::thread *mLoggingThread;
  bool mLoggingThreadOn;

  /* Checking statistics and decide switching */
  void get_stats(Stats &stats);
  void print_stats(Stats &stats);
  void check_and_decide_switching(Stats &stats);

private:
  /* Check policy-driven condition of increase/decrease */
  bool check_increase_adapter(const Stats &stats);
  bool check_decrease_adapter(const Stats &stats);

  /* Check condition of increase/decrease */
  bool is_increaseable(void);
  bool is_decreaseable(void);

  /* Get payoff points */
  int get_init_energy_payoff_point(void);
  int get_idle_energy_payoff_point(int avg_arrival_time_us);
  int get_init_latency_payoff_point(void);

  /* Increase/decrease adapter */
  bool increase_adapter(void);
  bool decrease_adapter(void);

  /* Policy-related */
  /* CoolSpots Policy */
  int mBandwidthWhenIncreasing;
  int mDecreasingCheckCount;

public:
  /* Mode getter */
  NSMode get_mode(void) {
    std::unique_lock<std::mutex> lck(this->mModeLock);
    NSMode mode = this->mMode;
    return mode;
  }

  /* Mode setter */
  void set_mode(NSMode new_mode) {
    std::unique_lock<std::mutex> lck(this->mModeLock);
    NSMode old_mode = this->mMode;
    this->mMode = new_mode;
    
    LOG_IMP("Network Monitor Mode Change: %d -> %d", old_mode, new_mode);
  }

private:
  /* Mode */
  NSMode mMode;
  std::mutex mModeLock;

public:
  /* Singleton */
  static NetworkMonitor *singleton(void) {
    if (NetworkMonitor::sSingleton == NULL) {
      NetworkMonitor::sSingleton = new NetworkMonitor();
    }
    return NetworkMonitor::sSingleton;
  }

private:
  /* Singleton */
  static NetworkMonitor *sSingleton;
  NetworkMonitor(void) {
    this->mMonitorThreadOn = false;
    this->mMonitorThread = NULL;
    this->mBandwidthWhenIncreasing = 0;
    this->mDecreasingCheckCount = 0;
    this->set_mode(NSMode::kNSModeEnergyAware);
  }
}; /* class NetworkMonitor */
} /* namespace sc */

#endif /* !defined(__NETWORK_MONITOR_H__) */