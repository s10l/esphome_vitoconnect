/*
  optolink.cpp - Connect Viessmann heating devices via Optolink to ESPhome

  Copyright (C) 2023  Philipp Danner

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "vitoconnect.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect";

void VitoConnect::setup() {

    this->check_uart_settings(4800, 2, uart::UART_CONFIG_PARITY_EVEN, 8);

    ESP_LOGD(TAG, "Starting optolink with protocol: %s", this->protocol.c_str());
    if (this->protocol.compare("P300") == 0) {
        _optolink = new OptolinkP300(this);
    } else if (this->protocol.compare("KW") == 0) {
        _optolink = new OptolinkKW(this);
    } else {
      ESP_LOGW(TAG, "Unknown protocol.");
    }

    // optimize datapoint list
    _datapoints.shrink_to_fit();

    if (_optolink) {

      // add onData and onError callbacks
      _optolink->onData(&VitoConnect::_onData);
      _optolink->onError(&VitoConnect::_onError);
      _optolink->onQueueEmpty(&VitoConnect::_onQueueEmpty);
      
      // set initial state
      _optolink->begin();

    } else {
      ESP_LOGW(TAG, "Not able to initialize VitoConnect");
    }

    if (this->_datapoints.size() == 0) {
      ESP_LOGW(TAG, "No datapoints registered!");
    } else {
      ESP_LOGD(TAG, "Registered %d datapoints", this->_datapoints.size());
    }

    if (this->_datapoints.size() > VITOWIFI_MAX_QUEUE_LENGTH) {
      ESP_LOGE(TAG, "Number of registered datapoints (%d) exceeds max. queue length (%d). Some datapoints may be skipped during update cycles.", 
                this->_datapoints.size(), VITOWIFI_MAX_QUEUE_LENGTH);
    }
}

void VitoConnect::register_datapoint(Datapoint *datapoint) {
    ESP_LOGD(TAG, "Adding datapoint with address %x and length %d", datapoint->getAddress(), datapoint->getLength());
    this->_datapoints.push_back(datapoint);
}

void VitoConnect::loop() {
    _optolink->loop();
}

void VitoConnect::update() {
  ESP_LOGD(TAG, "Schedule sensor update (Every %d ms)", this->get_update_interval());
  if (last_update_start != 0) {
    ESP_LOGE(TAG, "Previous update cycle not finished yet! Skipping this update.");
    return;
  }
  last_update_start = millis();
  total_reads = 0;
    
  uint32_t avg_read_time = getAverageReadTime();
  ESP_LOGD(TAG, "Average read time: %d ms, Number of sensors: %d, max. queue size %d, expected read time: %d ms", 
    avg_read_time, this->_datapoints.size(), VITOWIFI_MAX_QUEUE_LENGTH, avg_read_time * this->_datapoints.size());
  
  std::vector<Datapoint*> sorted_datapoints = getSortedDatapointsByPriority();
  
  uint32_t estimated_time = 0;
  uint32_t queued_count = 0;
  
  for (Datapoint* dp : sorted_datapoints) {
    uint32_t time_remaining = this->get_update_interval() - estimated_time;
    
    if (shouldQueueDatapoint(dp, time_remaining, avg_read_time)) {
      if (queueDatapointRead(dp)) {
        estimated_time += avg_read_time;
        queued_count++;
        ESP_LOGV(TAG, "Queued datapoint 0x%04X (priority %d), est_time now: %d ms", 
                 dp->getAddress(), dp->getPriority(), estimated_time);
      }
    } else {
      ESP_LOGD(TAG, "Skipping datapoint 0x%04X (priority %d) - insufficient time (remaining: %d ms, need: %d ms)", 
               dp->getAddress(), dp->getPriority(), time_remaining, avg_read_time * 2);
    }
  }
  
  ESP_LOGD(TAG, "Queued %d datapoints (estimated time: %d ms)", queued_count, estimated_time);
}

uint32_t VitoConnect::getAverageReadTime() {
  if (total_read_time > 0) {
    return total_read_time / this->_datapoints.size();
  }
// Calculate read time based on 4800 baud. This is only necessary for the first run.
// Send: 5 bytes (0x01F7000002) + 2 stop bit = 8 bits per byte * 7 = 56 bits
// Receive: 2 bytes (0x0000) + 2 stop bit = 8 bits per byte * 2 = 16 bits
// Total: 56 + 16 = 72 bits / 4800 baud = 15 ms
// Add protocol overhead (~50ms) for typical Viessmann response time
// per read (15ms transmission + ~50ms protocol overhead) = ~65ms
  return 65;
}

std::vector<Datapoint*> VitoConnect::getSortedDatapointsByPriority() {
  std::vector<Datapoint*> sorted = this->_datapoints;
  // Sort by priority: high priority first (lower number = higher priority)
  // Priority 1 should be queued first, then 2, then 3
  std::sort(sorted.begin(), sorted.end(), 
    [](Datapoint* a, Datapoint* b) {
      return a->getPriority() < b->getPriority();
    });
  return sorted;
}

bool VitoConnect::shouldQueueDatapoint(Datapoint* dp, uint32_t time_remaining, uint32_t avg_read_time) {
  // Always queue high priority datapoints (priority 1)
  if (dp->getPriority() == 1) {
    return true;
  }
  
  // For medium/low priority, check if we have enough time
  // Reserve at least 2x avg_read_time as safety margin
  return time_remaining >= avg_read_time * 2;
}

bool VitoConnect::queueDatapointRead(Datapoint* dp) {
  CbArg* arg = new CbArg(this, dp);
  
  if (_optolink->read(dp->getAddress(), dp->getLength(), reinterpret_cast<void*>(arg))) {
    return true;
  }
  
  delete arg;
  return false;
}

void VitoConnect::_onData(uint8_t* data, uint8_t len, void* arg) {
  CbArg* cbArg = reinterpret_cast<CbArg*>(arg);
  
  cbArg->v->total_reads++;
  uint32_t total_read_duration = millis() - cbArg->v->last_update_start;
  uint32_t avg_read_time = total_read_duration / cbArg->v->total_reads;
    
  ESP_LOGV(TAG, "Read completed. Avg read time: %d ms, Total reads: %d, Total duration: %d ms", 
             avg_read_time, cbArg->v->total_reads, total_read_duration);
  
  
  cbArg->dp->decode(data, len, cbArg->dp);
  delete cbArg;
}

void VitoConnect::_onError(uint8_t error, void* arg) {
  ESP_LOGD(TAG, "Error received: %d", error);
  CbArg* cbArg = reinterpret_cast<CbArg*>(arg);
  if (cbArg->v->_onErrorCb) cbArg->v->_onErrorCb(error, cbArg->dp);
  delete cbArg;
}

void VitoConnect::_onQueueEmpty(void *arg) {
  ESP_LOGV(TAG, "Optolink queue is empty.");
  CbArg* cbArg = reinterpret_cast<CbArg*>(arg);
  if (cbArg->v->last_update_start > 0) {
    cbArg->v->total_read_time = millis() - cbArg->v->last_update_start;
    ESP_LOGD(TAG, "Update cycle completed in %d ms, %d datapoints read", cbArg->v->total_read_time, cbArg->v->total_reads);
    cbArg->v->last_update_start = 0;
  }
}

}  // namespace vitoconnect
}  // namespace esphome
