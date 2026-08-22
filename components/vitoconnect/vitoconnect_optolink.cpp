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

#include "vitoconnect_optolink.h"
#include "esphome/core/log.h"
#include <cstddef>
#include <cstdint>

namespace esphome {
namespace vitoconnect {

namespace {
static constexpr uint32_t HANDSHAKE_BACKOFF_MS[] = {1000UL, 2000UL, 5000UL, 10000UL, 30000UL};
static constexpr uint8_t HANDSHAKE_FAILURE_PAUSE_THRESHOLD = 5;

inline bool is_time_in_future_(uint32_t now, uint32_t deadline) {
  return deadline != 0 && static_cast<int32_t>(now - deadline) < 0;
}
}  // namespace

Optolink::Optolink(uart::UARTDevice* uart) :
  _uart(uart),
  _queue(VITOWIFI_MAX_QUEUE_LENGTH),
  _onData(nullptr),
  _onError(nullptr),
  _onDataNoArg(nullptr),
  _onErrorNoArg(nullptr),
  _handshake_failures(0),
  _handshake_retry_at(0),
  _polling_paused(false) {}

Optolink::~Optolink() {
  // nothing to do
}

void Optolink::onData(void (*callback)(uint8_t* data, uint8_t len)) {
  _onDataNoArg = callback;
  _onData = nullptr;
}

void Optolink::onData(OnDataArgCallback callback) {
  _onData = callback;
  _onDataNoArg = nullptr;
}

void Optolink::onError(void (*callback)(uint8_t error)) {
  _onErrorNoArg = callback;
  _onError = nullptr;
}

void Optolink::onError(OnErrorArgCallback callback) {
  _onError = callback;
  _onErrorNoArg = nullptr;
}

bool Optolink::read(uint16_t address, uint8_t length, void* arg) {
  if (_isPollingPaused()) {
    return false;
  }
  if (length == 0 || length > MAX_DP_LENGTH) {
    return false;
  }
  OptolinkDP dp(address, length, false, nullptr, arg);
  return _queue.push(dp);
}

bool Optolink::write(uint16_t address, uint8_t length, uint8_t* data, void* arg) {
  if (_isPollingPaused()) {
    return false;
  }
  if (length == 0 || length > MAX_DP_LENGTH || data == nullptr) {
    return false;
  }
  OptolinkDP dp(address, length, true, data, arg);
  return _queue.push(dp);
}

void Optolink::_tryOnData(uint8_t* data, uint8_t len) {
  OptolinkDP* dp = _queue.front();
  if (dp == nullptr) return;
  if (_onData) _onData(data, len, dp->arg);
  else if (_onDataNoArg) _onDataNoArg(data, len);
  _queue.pop();
}

void Optolink::_tryOnError(uint8_t error) {
  OptolinkDP* dp = _queue.front();
  if (dp == nullptr) return;
  if (_onError) _onError(error, dp->arg);
  else if (_onErrorNoArg) _onErrorNoArg(error);
  _queue.pop();
}

bool Optolink::_isHandshakeBackoffActive(uint32_t now) const {
  return is_time_in_future_(now, _handshake_retry_at);
}

bool Optolink::_isPollingPaused() const {
  return _polling_paused;
}

void Optolink::_markHandshakeFailure(const char *tag, uint32_t now) {
  if (_handshake_failures < 0xFF) {
    ++_handshake_failures;
  }

  constexpr size_t kBackoffCount = sizeof(HANDSHAKE_BACKOFF_MS) / sizeof(HANDSHAKE_BACKOFF_MS[0]);
  size_t backoff_index = static_cast<size_t>(_handshake_failures == 0 ? 0 : _handshake_failures - 1);
  if (backoff_index >= kBackoffCount) {
    backoff_index = kBackoffCount - 1;
  }

  const uint32_t backoff_ms = HANDSHAKE_BACKOFF_MS[backoff_index];
  _handshake_retry_at = now + backoff_ms;
  if (_handshake_retry_at == 0) {
    _handshake_retry_at = 1;
  }

  ESP_LOGW(tag, "Handshake failed (%u consecutive), next retry in %lu ms",
           static_cast<unsigned>(_handshake_failures),
           static_cast<unsigned long>(backoff_ms));

  if (_handshake_failures >= HANDSHAKE_FAILURE_PAUSE_THRESHOLD) {
    const bool was_paused = _polling_paused;
    _polling_paused = true;
    if (!was_paused) {
      ESP_LOGW(tag, "Handshake failure threshold reached; pausing polling until handshake recovers");

      const size_t queued = _queue.size();
      if (queued > 0) {
        ESP_LOGW(tag, "Clearing %u queued requests while paused", static_cast<unsigned>(queued));
        _clearQueueWithError(TIMEOUT);
      }
    }
  }
}

void Optolink::_markHandshakeSuccess() {
  _handshake_failures = 0;
  _handshake_retry_at = 0;
  _polling_paused = false;
}

void Optolink::_clearQueueWithError(uint8_t error) {
  while (_queue.size() > 0) {
    _tryOnError(error);
  }
}

}  // namespace vitoconnect
}  // namespace esphome
