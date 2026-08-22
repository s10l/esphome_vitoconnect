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

// Modified by Stefan Bickel, 2026-01-08: Prioritized writes in update() and added verification read.

#include "vitoconnect.h"

#include <cstdio>
#include <cstring>

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect";

namespace {
const char *optolink_error_name_(uint8_t error) {
  switch (error) {
    case TIMEOUT:
      return "TIMEOUT";
    case LENGTH:
      return "LENGTH";
    case NACK:
      return "NACK";
    case CRC:
      return "CRC";
    case VITO_ERROR:
      return "VITO_ERROR";
    default:
      return "UNKNOWN";
  }
}
}  // namespace

void VitoConnect::setup() {

    this->check_uart_settings(4800, 2, uart::UART_CONFIG_PARITY_EVEN, 8);

    ESP_LOGI(TAG, "Starting optolink with protocol: %s", this->protocol.c_str());
    _optolink = nullptr;
    if (this->protocol.compare("P300") == 0) {
        _optolink = new OptolinkP300(this);
    } else if (this->protocol.compare("KW") == 0) {
        _optolink = new OptolinkKW(this);
    } else if (this->protocol.compare("GWG") == 0) {
        _optolink = new OptolinkGWG(this);
    } else {
      ESP_LOGW(TAG, "Unknown protocol.");
    }

    // optimize datapoint list
    _datapoints.shrink_to_fit();

    if (_optolink) {

      // add onData and onError callbacks
      _optolink->onData(&VitoConnect::_onData);
      _optolink->onError(&VitoConnect::_onError);

      // set initial state
      _optolink->begin();

    } else {
      ESP_LOGW(TAG, "Not able to initialize VitoConnect");
    }
}

void VitoConnect::register_datapoint(Datapoint *datapoint) {
    ESP_LOGD(TAG, "Register datapoint addr=0x%04X len=%u",
             datapoint->getAddress(), static_cast<unsigned>(datapoint->getLength()));
    this->_datapoints.push_back(datapoint);
}

void VitoConnect::loop() {
    if (_optolink) {
      _optolink->loop();
    }
}

void VitoConnect::update() {
  if (!_optolink) {
    ESP_LOGW(TAG, "Optolink not initialized; skipping update");
    return;
  }

  uint32_t queued_writes = 0;
  uint32_t queued_reads = 0;
  ESP_LOGI(TAG, "Starting poll cycle: datapoints=%u",
           static_cast<unsigned>(this->_datapoints.size()));

  // prioritize writes over reads
  for (Datapoint* dp : this->_datapoints) {
    if (dp->getLastUpdate() == 0) continue;
    if (dp->isWriteInFlight()) {
      ESP_LOGD(TAG, "Skip write enqueue: addr=0x%04X write already in-flight", dp->getAddress());
      continue;
    }

    if (dp->getWriteFailCount() >= this->max_write_failures_) {
      const uint32_t current_last_update = dp->getLastUpdate();
      const uint32_t fail_seq = dp->getWriteFailSeq();
      if (current_last_update != 0 && fail_seq == 0) {
        ESP_LOGW(TAG, "Write retry budget reached for addr=0x%04X (max=%u, failure_seq=unknown); keeping pending seq=%u",
                 dp->getAddress(), this->max_write_failures_, current_last_update);
        dp->resetWriteFailCount();
        dp->clearVerifyPending();
      } else if (current_last_update != 0 && fail_seq != 0 && current_last_update != fail_seq) {
        ESP_LOGW(TAG, "Write retry budget reached for addr=0x%04X (max=%u, stale seq=%u); keeping newer pending seq=%u",
                 dp->getAddress(), this->max_write_failures_, fail_seq, current_last_update);
        dp->resetWriteFailCount();
        dp->clearVerifyPending();
      } else {
        ESP_LOGE(TAG, "Write failed too often for addr=0x%04X (max=%u, seq=%u); clearing dirty flag",
                 dp->getAddress(), this->max_write_failures_, fail_seq);
        dp->clearLastUpdate();
        dp->resetWriteFailCount();
        dp->clearVerifyPending();
        dp->setWriteInFlight(false);
        continue;
      }
    }

    ESP_LOGI(TAG, "Datapoint 0x%04X marked dirty; scheduling write", dp->getAddress());

    const uint8_t dp_len = dp->getLength();
    if (dp_len == 0 || dp_len > MAX_DP_LENGTH) {
      ESP_LOGE(TAG, "Invalid write datapoint definition: addr=0x%04X len=%u",
               dp->getAddress(), static_cast<unsigned>(dp_len));
      continue;
    }

    uint8_t data[MAX_DP_LENGTH] = {0};
    dp->encode(&data[0], dp_len);

    // write the modified datapoint
    CbArg* writeCbArg = new CbArg(this, dp, true, dp->getLastUpdate());
    if (this->verify_writes_) {
      writeCbArg->has_exp = true;
      writeCbArg->exp_len = dp_len;
      memcpy(writeCbArg->exp, data, dp_len);
    }
    ESP_LOGD(TAG, "Queue write addr=0x%04X len=%u seq=%u verify=%s",
             dp->getAddress(),
             static_cast<unsigned>(dp_len),
             static_cast<unsigned>(dp->getLastUpdate()),
             this->verify_writes_ ? "on" : "off");
    if (!_optolink->write(dp->getAddress(), dp_len, data, reinterpret_cast<void*>(writeCbArg))) {
      ESP_LOGW(TAG, "Failed to queue write addr=0x%04X len=%u", dp->getAddress(), static_cast<unsigned>(dp_len));
      delete writeCbArg;
      return;
    }
    ESP_LOGD(TAG, "Queued write addr=0x%04X len=%u", dp->getAddress(), static_cast<unsigned>(dp_len));
    ++queued_writes;
    dp->setWriteInFlight(true);
  }

  for (Datapoint* dp : this->_datapoints) {
      // Never poll-read a datapoint while a local value is pending to be written.
      // Otherwise the stale controller value can overwrite the requested state
      // before retries happen.
      if (dp->isWriteInFlight()) {
          ESP_LOGD(TAG, "Skip read enqueue: addr=0x%04X write in-flight", dp->getAddress());
          continue;
      }
      if (dp->getLastUpdate() != 0) {
          ESP_LOGD(TAG, "Skip read enqueue: addr=0x%04X pending write seq=%u",
                   dp->getAddress(), dp->getLastUpdate());
          continue;
      }

      const uint8_t dp_len = dp->getLength();
      if (dp_len == 0 || dp_len > MAX_DP_LENGTH) {
          ESP_LOGE(TAG, "Invalid read datapoint definition: addr=0x%04X len=%u",
                   dp->getAddress(), static_cast<unsigned>(dp_len));
          continue;
      }

      CbArg* arg = new CbArg(this, dp);
      ESP_LOGD(TAG, "Queue read addr=0x%04X len=%u", dp->getAddress(), static_cast<unsigned>(dp_len));
      if (_optolink->read(dp->getAddress(), dp_len, reinterpret_cast<void*>(arg))) {
          ESP_LOGD(TAG, "Queued read addr=0x%04X len=%u", dp->getAddress(), static_cast<unsigned>(dp_len));
          ++queued_reads;
      } else {
          ESP_LOGW(TAG, "Failed to queue read addr=0x%04X len=%u", dp->getAddress(), static_cast<unsigned>(dp_len));
          delete arg;
      }
  }
  ESP_LOGI(TAG, "Completed poll cycle: queued writes=%u queued reads=%u",
           static_cast<unsigned>(queued_writes), static_cast<unsigned>(queued_reads));
}

void VitoConnect::_onData(uint8_t* data, uint8_t len, void* arg) {
  if (arg == nullptr) {
    ESP_LOGW(TAG, "Optolink onData callback invoked with null arg");
    return;
  }
  CbArg* cbArg = reinterpret_cast<CbArg*>(arg);

  if (cbArg->w) {
    ESP_LOGI(TAG, "Write completed addr=0x%04X len=%u seq=%u",
             cbArg->dp->getAddress(),
             static_cast<unsigned>(len),
             static_cast<unsigned>(cbArg->la));
    ESP_LOGD(TAG, "Write response payload addr=0x%04X len=%u", cbArg->dp->getAddress(), static_cast<unsigned>(len));
    cbArg->dp->setWriteInFlight(false);
    const uint32_t current_last_update = cbArg->dp->getLastUpdate();
    if (current_last_update == 0) {
      // already not dirty
      ESP_LOGD(TAG, "Write completion for addr=0x%04X found no dirty flag", cbArg->dp->getAddress());
    } else if (current_last_update == cbArg->la) {
      // Only clear dirty flag if nothing changed since this write was queued.
      cbArg->dp->clearLastUpdate();
      if (cbArg->v->verify_writes_ && cbArg->has_exp) {
        cbArg->dp->setVerifyExpected(cbArg->la, cbArg->exp, cbArg->exp_len);
      } else {
        cbArg->dp->resetWriteFailCount();
      }
    } else {
      // A newer change happened while this write was in flight. Keep dirty so it will be written again.
      ESP_LOGD(TAG, "Datapoint changed during write: addr=0x%04X queued seq=%u current seq=%u; keeping dirty",
               cbArg->dp->getAddress(), cbArg->la, current_last_update);
      cbArg->dp->clearVerifyPending();
      cbArg->dp->resetWriteFailCount();
    }
  } else { // ignore read responses only while the datapoint write is in flight
    if (cbArg->dp->isWriteInFlight()) {
      ESP_LOGD(TAG, "Ignoring read response: addr=0x%04X write in-flight", cbArg->dp->getAddress());
      delete cbArg;
      return;
    }
    if (cbArg->dp->getLastUpdate() != 0) {
      ESP_LOGD(TAG, "Ignoring read response: addr=0x%04X pending write seq=%u",
               cbArg->dp->getAddress(), cbArg->dp->getLastUpdate());
      delete cbArg;
      return;
    }

    if (cbArg->v->verify_writes_ && cbArg->dp->isVerifyPending()) {
      const uint8_t vlen = cbArg->dp->getVerifyLength();
      bool match = (vlen == len) && (memcmp(cbArg->dp->getVerifyExpected(), data, vlen) == 0);

      if (!match) {
        // log expected vs actual
        char exp_hex[3 * MAX_DP_LENGTH + 1] = {0};
        char act_hex[3 * MAX_DP_LENGTH + 1] = {0};
        const uint8_t exp_dump_len = (vlen < MAX_DP_LENGTH) ? vlen : MAX_DP_LENGTH;
        const uint8_t act_dump_len = (len < MAX_DP_LENGTH) ? len : MAX_DP_LENGTH;
        for (uint8_t i = 0; i < exp_dump_len; i++) {
          snprintf(&exp_hex[i * 3], 4, "%02X ", cbArg->dp->getVerifyExpected()[i]);
        }
        for (uint8_t i = 0; i < act_dump_len; i++) {
          snprintf(&act_hex[i * 3], 4, "%02X ", data[i]);
        }
        ESP_LOGE(TAG, "Write verify mismatch: addr=0x%04X expected[%u]=%s got[%u]=%s",
                 cbArg->dp->getAddress(), vlen, exp_hex, len, act_hex);
        // Re-mark dirty so update() can retry writing the desired value.
        uint32_t retry_seq = cbArg->dp->getVerifySeq();
        if (retry_seq == 0) {
          retry_seq = millis();
          if (retry_seq == 0) retry_seq = 1;
        }
        cbArg->dp->incWriteFailCount(retry_seq);
        cbArg->dp->setLastUpdate(retry_seq);
        cbArg->dp->clearVerifyPending();
        delete cbArg;
        return;
      } else {
        ESP_LOGD(TAG, "Write verify OK: addr=0x%04X", cbArg->dp->getAddress());
        cbArg->dp->resetWriteFailCount();
        cbArg->dp->clearVerifyPending();
      }
    }

    ESP_LOGD(TAG, "Read response addr=0x%04X len=%u", cbArg->dp->getAddress(), static_cast<unsigned>(len));
    cbArg->dp->decode(data, len, cbArg->dp);
  }

  delete cbArg;
}

void VitoConnect::_onError(uint8_t error, void* arg) {
  const char *error_name = optolink_error_name_(error);
  if (arg == nullptr) {
    ESP_LOGW(TAG, "Optolink error %u (%s) with null arg",
             static_cast<unsigned>(error), error_name);
    return;
  }
  CbArg* cbArg = reinterpret_cast<CbArg*>(arg);
  if (cbArg->dp != nullptr) {
    if (error == TIMEOUT) {
      ESP_LOGD(TAG, "Optolink error %u (%s) during %s addr=0x%04X len=%u",
               static_cast<unsigned>(error), error_name,
               cbArg->w ? "write" : "read",
               cbArg->dp->getAddress(),
               static_cast<unsigned>(cbArg->dp->getLength()));
    } else {
      ESP_LOGW(TAG, "Optolink error %u (%s) during %s addr=0x%04X len=%u",
               static_cast<unsigned>(error), error_name,
               cbArg->w ? "write" : "read",
               cbArg->dp->getAddress(),
               static_cast<unsigned>(cbArg->dp->getLength()));
    }
  } else {
    ESP_LOGW(TAG, "Optolink error %u (%s) during %s with null datapoint",
             static_cast<unsigned>(error), error_name, cbArg->w ? "write" : "read");
    if (cbArg->v->_onErrorCb) cbArg->v->_onErrorCb(error, nullptr);
    delete cbArg;
    return;
  }

  if (cbArg->w) {
    cbArg->dp->setWriteInFlight(false);
    cbArg->dp->incWriteFailCount(cbArg->la);
    cbArg->dp->clearVerifyPending();
    const uint8_t fail_count = cbArg->dp->getWriteFailCount();
    const uint32_t current_last_update = cbArg->dp->getLastUpdate();
    if (fail_count >= cbArg->v->max_write_failures_) {
      if (current_last_update == cbArg->la && current_last_update != 0) {
        ESP_LOGE(TAG, "Write failed repeatedly: addr=0x%04X failures=%u max=%u; clearing dirty flag",
                 cbArg->dp->getAddress(), fail_count, cbArg->v->max_write_failures_);
        cbArg->dp->clearLastUpdate();
        cbArg->dp->resetWriteFailCount();
      } else {
        // Failure budget was exhausted for an older queued write. Keep a newer
        // pending write dirty and give it a fresh retry budget.
        if (current_last_update != 0) {
          ESP_LOGW(TAG, "Write reached max failures for stale seq: addr=0x%04X stale=%u current=%u; keeping newer pending write",
                   cbArg->dp->getAddress(), cbArg->la, current_last_update);
        }
        cbArg->dp->resetWriteFailCount();
      }
    }
  }
  if (cbArg->v->_onErrorCb) cbArg->v->_onErrorCb(error, cbArg->dp);
  delete cbArg;
}

}  // namespace vitoconnect
}  // namespace esphome
