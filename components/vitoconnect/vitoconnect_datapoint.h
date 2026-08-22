/* VitoWiFi

Copyright 2019 Bert Melis

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

// Modified by Stefan Bickel, 2026-01-08: Added _last_update member to track external datapoint modification time.

#pragma once

#include <stdint.h>
#include <assert.h>
#include <functional>
#include <string.h>  // for memcpy

#include "vitoconnect_optolink.h"

namespace esphome {
namespace vitoconnect {

class Datapoint {

 public:
  Datapoint();
  virtual ~Datapoint();

  void setAddress(uint16_t address) {  this->_address = address; };
  uint16_t getAddress() { return this->_address; };

  void setLength(uint8_t length) {  this->_length = length; };
  uint8_t getLength() { return this->_length; };

  static void onData(std::function<void(uint8_t[], uint8_t, Datapoint* dp)> callback);
  void onError(uint8_t, Datapoint* dp);

  virtual void encode(uint8_t* raw, uint8_t length);
  virtual void encode(uint8_t* raw, uint8_t length, void* data);
  virtual void decode(uint8_t* data, uint8_t length, Datapoint* dp = nullptr);

  uint32_t getLastUpdate() { return _last_update; };
  void setLastUpdate(uint32_t last_update) { this->_last_update = last_update; }
  void clearLastUpdate() { this->_last_update = 0; }

  // --- Write state tracking -------------------------------------------------
  // `_last_update` indicates that an external caller (e.g. an ESPHome entity)
  // changed the datapoint and that it should be written back to the controller.
  // `_write_in_flight` indicates that a write request has been queued/sent and
  // we are awaiting its completion (success or error).
  bool isWriteInFlight() const { return this->_write_in_flight; }
  void setWriteInFlight(bool v) { this->_write_in_flight = v; }

  // Consecutive write failures (protocol errors, timeouts, verify mismatches).
  uint8_t getWriteFailCount() const { return this->_write_fail_count; }
  uint32_t getWriteFailSeq() const { return this->_write_fail_seq; }
  void resetWriteFailCount() {
    this->_write_fail_count = 0;
    this->_write_fail_seq = 0;
  }
  void incWriteFailCount(uint32_t seq = 0) {
    if (seq != 0) {
      if (this->_write_fail_seq != 0 && this->_write_fail_seq != seq) {
        // New queued write sequence: start a fresh failure budget.
        this->_write_fail_count = 0;
      }
      this->_write_fail_seq = seq;
    }
    if (this->_write_fail_count < 255) this->_write_fail_count++;
  }

  // --- Optional write verification -----------------------------------------
  // When enabled, the hub can store the raw bytes it attempted to write and
  // compare them with the next read-back of the datapoint.
  bool isVerifyPending() const { return this->_verify_pending; }
  uint32_t getVerifySeq() const { return this->_verify_seq; }
  uint8_t getVerifyLength() const { return this->_verify_length; }
  const uint8_t* getVerifyExpected() const { return this->_verify_expected; }
  void clearVerifyPending() {
    this->_verify_pending = false;
    this->_verify_seq = 0;
    this->_verify_length = 0;
    memset(this->_verify_expected, 0, sizeof(this->_verify_expected));
  }
  void setVerifyExpected(uint32_t seq, const uint8_t* raw, uint8_t len) {
    this->_verify_pending = true;
    this->_verify_seq = seq;
    if (len > kMaxDpLength) len = kMaxDpLength;
    this->_verify_length = len;
    memset(this->_verify_expected, 0, sizeof(this->_verify_expected));
    if (raw != nullptr && len > 0) memcpy(this->_verify_expected, raw, len);
  }

 protected:
  uint32_t _last_update = 0;
  bool _write_in_flight = false;
  uint8_t _write_fail_count = 0;
  uint32_t _write_fail_seq = 0;

  static constexpr uint8_t kMaxDpLength = MAX_DP_LENGTH;
  bool _verify_pending = false;
  uint32_t _verify_seq = 0;
  uint8_t _verify_length = 0;
  uint8_t _verify_expected[kMaxDpLength] = {0};
  uint16_t _address{0};
  uint8_t _length{0};
  static std::function<void(uint8_t[], uint8_t, Datapoint* dp)> _stdOnData;
};


}  // namespace vitoconnect
}  // namespace esphome
