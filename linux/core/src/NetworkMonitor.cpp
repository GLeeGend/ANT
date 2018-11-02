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

#include "../inc/NetworkMonitor.h"

#include "../inc/NetworkSwitcher.h"

#include "../../configs/ExpConfig.h"
#include "../../configs/NetworkSwitcherConfig.h"

#include <string.h>

using namespace sc;

NetworkMonitor *NetworkMonitor::sSingleton = NULL;

void NetworkMonitor::start(void) {
  this->mMonitorThreadOn = true;
  this->mMonitorThread =
      new std::thread(std::bind(&NetworkMonitor::monitor_thread, this));
  this->mMonitorThread->detach();

  this->start_logging();
}

void NetworkMonitor::stop(void) {
  this->mMonitorThreadOn = false;
  this->stop_logging();
}

void NetworkMonitor::start_logging(void) {
  this->mLoggingThreadOn = true;
  this->mLoggingThread =
      new std::thread(std::bind(&NetworkMonitor::logging_thread, this));
  this->mLoggingThread->detach();
}

void NetworkMonitor::stop_logging(void) { this->mLoggingThreadOn = false; }

void NetworkMonitor::monitor_thread(void) {
  int count = 0;
  while (this->mMonitorThreadOn) {
    // Get statistics
    Stats stats;
    this->get_stats(stats);

    // Print statistics
    this->print_stats(stats);

    // Check and switch
    // If the switcher is already switching,
    NSState switcher_state = NetworkSwitcher::singleton()->get_state();
    if (switcher_state == NSState::kNSStateReady) {
      this->check_and_decide_switching(stats);
    }

    usleep(NETWORK_MONITOR_SLEEP_USECS);
  }
}

void NetworkMonitor::logging_thread(void) {
  FILE *fp = ::fopen("./log", "w");
  if (fp == NULL) {
    LOG_ERR("Failed to open log file");
    return;
  }

  // Write header
  fprintf(fp, "Timeval(sec), EMA_ReqSize(B), EMA_IAT(ms), SQ_Length(B), "
              "Bandwidth(B/s), EMA_RTT(ms)\n");

  // Setting first timeval
  struct timeval first_tv;
  gettimeofday(&first_tv, NULL);
  long long first_tv_us =
      (long long)first_tv.tv_sec * 1000 * 1000 + (long long)first_tv.tv_usec;

  while (this->mLoggingThreadOn) {
    // Get statistics
    Core *core = Core::singleton();
 
    // Get relative now timeval
    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    long long now_tv_us =
        (long long)now_tv.tv_sec * 1000 * 1000 + (long long)now_tv.tv_usec;
    long long relative_now_tv_us = now_tv_us - first_tv_us;
    int relative_now_tv_sec = (int)(relative_now_tv_us) / (1000 * 1000);
    int relative_now_tv_usec = (int)(relative_now_tv_us) % (1000 * 1000);

    // Get EMA send RTT
    // Use get_stats() instead
    Stats stats;
    this->get_stats(stats);

    ::fprintf(fp, "%ld.%ld, %d, %3.3f, %d, %d, %3.3f\n", relative_now_tv_sec,
              relative_now_tv_usec, (int)stats.ema_send_request_size,
              (stats.ema_arrival_time_us / 1000), stats.now_queue_data_size,
              stats.now_total_bandwidth, stats.ema_send_rtt / 1000);
    ::fflush(fp);
    ::sleep(1);
  }

  ::fclose(fp);
}

void NetworkMonitor::print_stats(Stats &stats) {
#ifndef PRINT_NETWORK_MONITOR_STATISTICS
  return;
#else
  if (Core::singleton()->get_state() != CoreState::kCoreStateReady) {
    return;
  }

  /* Print statistics
   *  - EMA(Send Request Size (B))
   *  - EMA(Queue Arrival Speed (B/s))
   *
   *  - Queue Data Size
   *
   *  - Total Bandwidth
   *  - EMA(Arrival Time (sec))
   */
  switch (this->get_mode()) {
  case NSMode::kNSModeEnergyAware:
    printf("R: %dB (IAT: %3.3fms) => [Q: %dB ] => %dB/s // Energy ON:%dB "
           "OFF: %dB (%d) // RTT=%3.3fms\n",
           (int)stats.ema_send_request_size, (stats.ema_arrival_time_us / 1000),
           stats.now_queue_data_size, stats.now_total_bandwidth,
           this->get_init_energy_payoff_point(),
           this->get_idle_energy_payoff_point(stats.ema_arrival_time_us),
           this->mDecreasingCheckCount, stats.ema_send_rtt / 1000);
    break;
  case NSMode::kNSModeLatencyAware:
    printf("R: %dB (IAT: %3.3fms) => [Q: %dB ] => %dB/s // Latency ON:%dB "
           "OFF: N/A // RTT=%3.3fms\n",
           (int)stats.ema_send_request_size, (stats.ema_arrival_time_us / 1000),
           stats.now_queue_data_size, stats.now_total_bandwidth,
           this->get_init_latency_payoff_point(), stats.ema_send_rtt / 1000);
    break;
  case NSMode::kNSModeCapDynamic:
    printf("R: %dB (IAT: %3.3fms) => [Q: %dB ] => %dB/s // Cap-Dynamic // "
           "RTT=%3.3fms\n",
           (int)stats.ema_send_request_size, (stats.ema_arrival_time_us / 1000),
           stats.now_queue_data_size, stats.now_total_bandwidth,
           stats.ema_send_rtt / 1000);
    break;
  default:
    break;
  }
#endif
}

void NetworkMonitor::get_stats(Stats &stats) {
  // TODO: consider peer's request_speed, now_queue_data_size
  Core *core = Core::singleton();
  SegmentManager *sm = SegmentManager::singleton();

  /* Statistics used to print present status */
  stats.ema_queue_arrival_speed = sm->get_ema_queue_arrival_speed();

  /* Statistics used in CoolSpots Policy */
  stats.now_total_bandwidth = core->get_total_bandwidth();

  /* Statistics used in Energy-aware & Latency-aware Policy */
  stats.ema_send_request_size = core->get_ema_send_request_size();
  stats.ema_arrival_time_us = core->get_ema_send_arrival_time();
  stats.now_queue_data_size = sm->get_queue_data_size(kSQSendData) +
                              sm->get_queue_data_size(kSQSendControl) +
                              sm->get_failed_sending_queue_data_size();

  /* Statistics used to evaluate the policies */
  stats.ema_send_rtt = core->get_ema_send_rtt();
}

void NetworkMonitor::check_and_decide_switching(Stats &stats) {
  /* Determine Increasing/Decreasing adapter */
  if (this->check_increase_adapter(stats)) {
    /* CoolSpots: Maintain bandwidth when increasing */
    this->mBandwidthWhenIncreasing = stats.now_total_bandwidth;

    /* Increase Adapter */
    this->increase_adapter();
  } else if (this->check_decrease_adapter(stats)) {
    /* Decrease Adapter */
    this->decrease_adapter();
  }
}

int NetworkMonitor::get_init_energy_payoff_point(void) {
  /* 4634053B (4525KB) */
  return (int)((WFD_INIT_ENERGY) /
               (BT_TX_ENERGY_PER_1B - WFD_TX_ENERGY_PER_1B));
}
int NetworkMonitor::get_idle_energy_payoff_point(int ema_arrival_time_us) {
  /*
   * 229118B(224KB) at 1sec
   * 6873542B(6712KB) at 30sec
   */
  return (int)((WFD_IDLE_ENERGY_PER_1SEC * ema_arrival_time_us / 1000000) /
               (BT_TX_ENERGY_PER_1B - WFD_TX_ENERGY_PER_1B));
}

int NetworkMonitor::get_init_latency_payoff_point(void) {
  /* 612787B (598KB) */
  return (int)((WFD_INIT_LATENCY + WFD_TX_LATENCY_1KB_BASIS -
                BT_TX_LATENCY_1KB_BASIS) /
               (BT_TX_LATENCY_PER_1B - WFD_TX_LATENCY_PER_1B));
}

bool NetworkMonitor::check_increase_adapter(const Stats &stats) {
  /* Check the condition of adapter increase based on switching policy */
  if (!this->is_increaseable()) {
    return false;
  } else if (Core::singleton()->get_state() != kCoreStateReady) {
    return false;
  } else {
    switch (this->get_mode()) {
    case NSMode::kNSModeEnergyAware: {
      /*
       * Energy-aware Policy:
       *  - queue data size + EMA(send_request_size) > init energy payoff point
       */
      if (stats.ema_send_request_size + stats.now_queue_data_size >
          this->get_init_energy_payoff_point()) {
        return true;
      } else {
        return false;
      }
      break;
    } /* case NSMode::kNSModeEnergyAware */
    case NSMode::kNSModeLatencyAware: {
      /*
       * Latency-aware Policy:
       *  - queue data size + EMA(send_request_size) > init latency payoff point
       */
      if (stats.ema_send_request_size + stats.now_queue_data_size >
          this->get_init_latency_payoff_point()) {
        return true;
      } else {
        return false;
      }
      break;
    } /* case NSMode::kNSModeLatencyAware */
    case NSMode::kNSModeCapDynamic: {
      /*
       * Cap-dynamic Policy:
       */

      LOG_ERR("Unsupported mode!: %d", this->get_mode());
      return false;
      break;
    } /* case NSMode::kNSModeCapDynamic */
    default: {
      LOG_ERR("Unsupported mode!: %d", this->get_mode());
      return false;
      break;
    } /* default */
    } /* switch(this->get_mode()) */
  }
}

#define CHECK_DECREASING_OK_COUNT 10
bool NetworkMonitor::check_decrease_adapter(const Stats &stats) {
  /* Check the condition of adapter decrease based on switching policy */
  if (!this->is_decreaseable()) {
    return false;
  } else if (Core::singleton()->get_state() != kCoreStateReady) {
    return false;
  } else {
    switch (this->get_mode()) {
    case NSMode::kNSModeEnergyAware: {
      /*
       * Energy-aware Policy:
       *  - if(wfd_idle_energy < wfd_init_energy) EMA(send_request_size)
       *      > idle energy payoff point
       *  - if(wfd_idle_energy > wfd_init_energy) always false
       */
      bool wfd_off;
      int wfd_idle_energy =
          WFD_IDLE_ENERGY_PER_1SEC * stats.ema_arrival_time_us / 1000000;
      int queue_size = stats.now_queue_data_size;
      int bandwidth = stats.now_total_bandwidth;
      if (WFD_INIT_ENERGY < wfd_idle_energy) {
        wfd_off = true;
      } else {
        int next_request_size = stats.ema_send_request_size;
        int idle_energy_payoff_point =
            get_idle_energy_payoff_point(stats.ema_arrival_time_us);
        if (next_request_size < idle_energy_payoff_point) {
          wfd_off = true;
        } else {
          wfd_off = false;
        }
      }

      if (queue_size > 0) {
        // If queue is not empty, do not turn off WFD
        return false;
      } else if (wfd_off) {
        if ((bandwidth == 0) ||
            (stats.ema_send_request_size <
             this->get_idle_energy_payoff_point(stats.ema_arrival_time_us))) {
          this->mDecreasingCheckCount++;
          if (this->mDecreasingCheckCount >= CHECK_DECREASING_OK_COUNT) {
            this->mDecreasingCheckCount = 0;
            return true;
          } else {
            return false;
          }
        } else {
          this->mDecreasingCheckCount = 0;
          return false;
        }
      } else {
        this->mDecreasingCheckCount = 0;
        return false;
      }
      break;
    } /* case NSMode::kNSModeEnergyAware */
    case NSMode::kNSModeLatencyAware: {
      /*
       * Latency-aware Policy:
       *  - always false
       */
      return false;
      break;
    } /* case NSMode::kNSModeLatencyAware */
    case NSMode::kNSModeCapDynamic: {
      /*
       * Cap-dynamic Policy:
       */
      LOG_ERR("Unsupported mode!: %d", this->get_mode());
      break;
    } /* case NSMode::kNSModeCapDynamic */
    default: {
      LOG_ERR("Unsupported mode!: %d", this->get_mode());
      return false;
      break;
    }
    } /* switch(this->get_mode()) */
  }
}

bool NetworkMonitor::is_increaseable(void) {
  /* Check the minimum condition of adapter increase such as adapters' count
   */
  Core *core = Core::singleton();
  int adapter_count = core->get_adapter_count();
  int active_adapter_index = core->get_active_adapter_index();
  return ((adapter_count > 1) && (active_adapter_index < (adapter_count - 1)));
}
bool NetworkMonitor::is_decreaseable(void) {
  /* Check the minimum condition of adapter decrease such as adapters' count
   */
  Core *core = Core::singleton();
  int adapter_count = core->get_adapter_count();
  int active_adapter_index = core->get_active_adapter_index();
  return ((adapter_count > 1) && (active_adapter_index > 0));
}

bool NetworkMonitor::increase_adapter(void) {
  Core *core = Core::singleton();
  if (core->get_adapter_count() == 0) {
    LOG_ERR("No adapter is registered!");
    return false;
  } else if (!this->is_increaseable()) {
    LOG_WARN("Cannot increase adapter!");
    return false;
  }

  int prev_index = core->get_active_adapter_index();
  int next_index = prev_index + 1;

  return NetworkSwitcher::singleton()->switch_adapters(prev_index, next_index);
}

bool NetworkMonitor::decrease_adapter(void) {
  Core *core = Core::singleton();
  if (core->get_adapter_count() == 0) {
    LOG_ERR("No adapter is registered!");
    return false;
  } else if (!this->is_decreaseable()) {
    LOG_WARN("Cannot deccrease adapter!");
    return false;
  }

  int prev_index = core->get_active_adapter_index();
  int next_index = prev_index - 1;

  return NetworkSwitcher::singleton()->switch_adapters(prev_index, next_index);
}