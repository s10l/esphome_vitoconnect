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

#include "vitoconnect_optolinkP300.h"
#include <cstddef>

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect";
static constexpr uint32_t P300_RESET_ACK_TIMEOUT_MS = 5000UL;
static constexpr uint32_t P300_INIT_ACK_TIMEOUT_MS = 5000UL;
static constexpr uint32_t P300_ENABLE_RETRY_MIN_MS = 100UL;
static constexpr uint8_t P300_ACK_READ_BUDGET = 32;

static inline void drain_uart_(uart::UARTDevice *uart) {
  while (uart != nullptr && uart->available()) {
    (void) uart->read();
  }
}

inline uint8_t calcChecksum(const uint8_t* array, size_t length) {
  uint8_t sum = 0;
  for (size_t i = 1; i < length - 1; ++i) {  // start with second byte and end before checksum
    sum += array[i];
  }
  return sum;
}

inline bool checkChecksum(const uint8_t* array, size_t length) {
  return (array[length - 1] == calcChecksum(array, length));
}

OptolinkP300::OptolinkP300(uart::UARTDevice* uart) :
  Optolink(uart),
  _state(UNDEF),
  _lastMillis(0),
  _write(false),
  _rcvBuffer{0},
  _rcvBufferLen(0),
  _rcvLen(0),
  _initAckSawRx(false),
  _initAckLastRx(0),
  _initAckStartMs(0),
  _initAckLastEnableTxMs(0) {}

void OptolinkP300::begin() {
  _markHandshakeSuccess();
  _lastMillis = millis();
  _state = RESET;
}

void OptolinkP300::loop() {
  switch (_state) {
  case RESET:
    _reset();
    break;
  case RESET_ACK:
    _resetAck();
    break;
  case INIT:
    _init();
    break;
  case INIT_ACK:
    _initAck();
    break;
  case IDLE:
    _idle();
    break;
  case SEND:
    _send();
    break;
  case SEND_ACK:
    _sentAck();
    break;
  case RECEIVE:
    _receive();
    break;
  case RECEIVE_ACK:
    _receiveAck();
    break;
  default:
    // begin() not called
    break;
  }
  const uint32_t now = millis();
  const bool request_in_flight = (_queue.size() > 0) && (_state == SEND_ACK || _state == RECEIVE);
  if (request_in_flight && (now - _lastMillis > 5000UL)) {
    OptolinkDP *dp = _queue.front();
    ESP_LOGW(TAG, "TIMEOUT in state=%u addr=0x%04X len=%u",
             static_cast<unsigned>(_state),
             dp != nullptr ? dp->address : 0U,
             dp != nullptr ? static_cast<unsigned>(dp->length) : 0U);
    _tryOnError(TIMEOUT);
    _state = RESET;
    drain_uart_(_uart);
    _uart->flush();
    _lastMillis = now;
  }
  // TODO(@bertmelis): move timeouts here, clear queue on timeout
}

void OptolinkP300::_reset() {
  if (_isHandshakeBackoffActive(millis())) {
    return;
  }
  // Set communication with Vitotronic to defined state = reset to KW protocol
  drain_uart_(_uart);
  const uint8_t buff[] = {0x04};
  _uart->write_array(buff, sizeof(buff));
  _lastMillis = millis();
  _state = RESET_ACK;
}

void OptolinkP300::_resetAck() {
  for (uint8_t reads = 0; reads < P300_ACK_READ_BUDGET && _uart->available(); ++reads) {
    int rb = _uart->read();
    if (rb < 0) {
      break;
    }
    uint8_t b = static_cast<uint8_t>(rb);
    if (b == 0x05 || b == 0x06) {
      // received reset acknowledgment
      _lastMillis = millis();
      _state = INIT;
      return;
    }
  }
  const uint32_t now = millis();
  if (now - _lastMillis > P300_RESET_ACK_TIMEOUT_MS) {
    ESP_LOGW(TAG, "P300 reset ACK timeout after %lu ms",
             static_cast<unsigned long>(P300_RESET_ACK_TIMEOUT_MS));
    _markHandshakeFailure(TAG, now);
    _state = RESET;
  }
}

void OptolinkP300::_init() {
  drain_uart_(_uart);
  _initAckSawRx = false;
  _initAckLastRx = 0;
  const uint8_t buff[] = {0x16, 0x00, 0x00};
  _uart->write_array(buff, sizeof(buff));
  const uint32_t now = millis();
  _lastMillis = now;
  _initAckStartMs = now;
  _initAckLastEnableTxMs = now;
  _state = INIT_ACK;
}

void OptolinkP300::_initAck() {
  const uint32_t now = millis();
  for (uint8_t reads = 0; reads < P300_ACK_READ_BUDGET && _uart->available(); ++reads) {
    int rb = _uart->read();
    if (rb < 0) {
      break;
    }
    const uint8_t b = static_cast<uint8_t>(rb);
    _initAckSawRx = true;
    _initAckLastRx = b;
    if (b == 0x06) {
      // ACK received, moving to next state
      _lastMillis = millis();
      _markHandshakeSuccess();
      _state = IDLE;
      return;
    }
    if (b == 0x05) {
      const uint32_t retry_now = millis();
      if (retry_now - _initAckLastEnableTxMs >= P300_ENABLE_RETRY_MIN_MS) {
        const uint8_t enable[] = {0x16, 0x00, 0x00};
        _uart->write_array(enable, sizeof(enable));
        _initAckLastEnableTxMs = retry_now;
      }
      continue;
    }
    if (b == 0x15) {
      ESP_LOGW(TAG, "P300 enable got NACK (0x15), restarting handshake");
      _markHandshakeFailure(TAG, now);
      _state = RESET;
      return;
    }
  }
  if (now - _initAckStartMs > P300_INIT_ACK_TIMEOUT_MS) {
    if (_initAckSawRx) {
      ESP_LOGW(TAG, "P300 enable ACK timeout after %lu ms, last RX byte=0x%02X",
               static_cast<unsigned long>(P300_INIT_ACK_TIMEOUT_MS), _initAckLastRx);
    } else {
      ESP_LOGW(TAG, "P300 enable ACK timeout after %lu ms, no RX bytes",
               static_cast<unsigned long>(P300_INIT_ACK_TIMEOUT_MS));
    }
    _markHandshakeFailure(TAG, now);
    _state = RESET;
  }
}

void OptolinkP300::_idle() {
  // send INIT every 5 seconds to keep communication alive
  if (millis() - _lastMillis > 5 * 1000UL) {
    _state = INIT;
  }
  if (_queue.size() > 0) {
    _state = SEND;
  }
}

void OptolinkP300::_send() {
  uint8_t buff[MAX_DP_LENGTH + 8];
  drain_uart_(_uart);
  _rcvBufferLen = 0;
  _rcvLen = 0;
  memset(_rcvBuffer, 0, sizeof(_rcvBuffer));
  OptolinkDP* dp = _queue.front();
  if (dp == nullptr) {
    _state = IDLE;
    return;
  }
  uint8_t length = dp->length;
  uint16_t address = dp->address;
  if (dp->write) {
    // type is WRITE, has length of 8 chars + length of value
    buff[0] = 0x41;
    buff[1] = 5 + length;
    buff[2] = 0x00;
    buff[3] = 0x02;
    buff[4] = (address >> 8) & 0xFF;
    buff[5] = address & 0xFF;
    buff[6] = length;
    // add value to message
    memcpy(&buff[7], dp->data, length);
    buff[7 + length] = calcChecksum(buff, 8 + length);
    _uart->write_array(buff, 8 + length);
  } else {
    // type is READ
    // has fixed length of 8 chars
    buff[0] = 0x41;
    buff[1] = 0x05;
    buff[2] = 0x00;
    buff[3] = 0x01;
    buff[4] = (address >> 8) & 0xFF;
    buff[5] = address & 0xFF;
    buff[6] = length;
    buff[7] = calcChecksum(buff, 8);
    _uart->write_array(buff, 8);
  }
  _lastMillis = millis();
  _state = SEND_ACK;
}

void OptolinkP300::_sentAck() {
  if (_uart->available()) {
    int rb = _uart->read();
    if (rb < 0) return;
    uint8_t buff = static_cast<uint8_t>(rb);
    if (buff == 0x06) {  // transmit successful, moving to next state
      _lastMillis = millis();
      _state = RECEIVE;
      return;
    } else if (buff == 0x41) {
      memset(_rcvBuffer, 0, sizeof(_rcvBuffer));
      _rcvBuffer[0] = 0x41;
      _rcvBufferLen = 1;
      _rcvLen = 0;
      _lastMillis = millis();
      _state = RECEIVE;
      return;
    } else if (buff == 0x15) {  // transmit negatively acknowledged, return
                                // to IDLE
      _tryOnError(NACK);
      _lastMillis = millis();
      _state = IDLE;
      return;
    }
  }
}

void OptolinkP300::_receive() {
  while (_uart->available() != 0) {
    int rb = _uart->read();
    if (rb < 0) break;
    const uint8_t b = static_cast<uint8_t>(rb);

    if (_rcvBufferLen == 0) {
      if (b != 0x41) continue;  // resync to start byte
      _rcvBuffer[0] = b;
      _rcvBufferLen = 1;
      _rcvLen = 0;
      _lastMillis = millis();
      continue;
    }

    if (_rcvBufferLen >= sizeof(_rcvBuffer)) {
      _tryOnError(LENGTH);
      _rcvBufferLen = 0;
      _rcvLen = 0;
      memset(_rcvBuffer, 0, sizeof(_rcvBuffer));
      _state = RESET;
      return;
    }

    _rcvBuffer[_rcvBufferLen++] = b;
    _lastMillis = millis();

    if (_rcvBufferLen == 2) {
      const size_t total = static_cast<size_t>(_rcvBuffer[1]) + 3U;
      if (total < 8U || total > sizeof(_rcvBuffer)) {
        _rcvBufferLen = 0;
        _rcvLen = 0;
        memset(_rcvBuffer, 0, sizeof(_rcvBuffer));
        if (b == 0x41) {
          _rcvBuffer[0] = b;
          _rcvBufferLen = 1;
        }
        continue;
      }
      _rcvLen = total;
    }

    if (_rcvLen != 0 && _rcvBufferLen >= _rcvLen) {
      break;
    }
  }

  if (_rcvLen == 0 || _rcvBufferLen < _rcvLen) return;

  if (!checkChecksum(_rcvBuffer, _rcvLen)) {
    const uint8_t nack[] = {0x15};
    _uart->write_array(nack, sizeof(nack));
    _rcvBufferLen = 0;
    _rcvLen = 0;
    memset(_rcvBuffer, 0, sizeof(_rcvBuffer));
    _lastMillis = millis();
    return;
  }

  OptolinkDP* dp = _queue.front();
  if (dp == nullptr) {
    _rcvBufferLen = 0;
    _rcvLen = 0;
    memset(_rcvBuffer, 0, sizeof(_rcvBuffer));
    _state = IDLE;
    return;
  }

  const uint8_t msgid = _rcvBuffer[2] & 0x0F;
  const uint8_t fct = _rcvBuffer[3] & 0x1F;
  const uint8_t payload_len = _rcvBuffer[6];
  const uint16_t resp_addr =
      (static_cast<uint16_t>(_rcvBuffer[4]) << 8) |
      static_cast<uint16_t>(_rcvBuffer[5]);
  if (resp_addr != dp->address) {
    ESP_LOGW(TAG, "P300 response address mismatch: expected addr=0x%04X got addr=0x%04X",
             dp->address, resp_addr);
    _tryOnError(VITO_ERROR);
    _state = RECEIVE_ACK;
  } else if (msgid == 0x03) {
    if (payload_len > 0 && (static_cast<size_t>(payload_len) + 8U) <= _rcvLen) {
      ESP_LOGW(TAG, "P300 error report=0x%02X for addr=0x%04X", _rcvBuffer[7], dp->address);
    } else {
      ESP_LOGW(TAG, "P300 error report with invalid payload length %u", payload_len);
    }
    _tryOnError(VITO_ERROR);
    _state = RECEIVE_ACK;
  } else if (msgid != 0x01) {
    _tryOnError(VITO_ERROR);
    _state = RECEIVE_ACK;
  } else if (fct == 0x01) {
    if (dp->write) {
      ESP_LOGW(TAG, "P300 received READ response for WRITE request: addr=0x%04X", dp->address);
      _tryOnError(VITO_ERROR);
    } else if (payload_len != dp->length || (static_cast<size_t>(payload_len) + 8U) != _rcvLen) {
      _tryOnError(LENGTH);
    } else {
      _tryOnData(&_rcvBuffer[7], payload_len);
    }
    _state = RECEIVE_ACK;
  } else if (fct == 0x02) {
    if (!dp->write) {
      ESP_LOGW(TAG, "P300 received WRITE response for READ request: addr=0x%04X", dp->address);
      _tryOnError(VITO_ERROR);
    } else if ((payload_len != dp->length) || (_rcvLen != 8U)) {
      ESP_LOGW(TAG, "P300 write response length mismatch: addr=0x%04X resp_len=%u expected=%u frame=%u",
               dp->address, payload_len, dp->length, static_cast<unsigned>(_rcvLen));
      _tryOnError(LENGTH);
    } else {
      _tryOnData(dp->data, dp->length);
    }
    _state = RECEIVE_ACK;
  } else {
    _tryOnError(VITO_ERROR);
    _state = RECEIVE_ACK;
  }

  _rcvBufferLen = 0;
  _rcvLen = 0;
  memset(_rcvBuffer, 0, sizeof(_rcvBuffer));
}

void OptolinkP300::_receiveAck() {
  const uint8_t buff[] = {0x06};
  _uart->write_array(buff, sizeof(buff));
  _lastMillis = millis();
  _state = IDLE;
}

}  // namespace vitoconnect
}  // namespace esphome
