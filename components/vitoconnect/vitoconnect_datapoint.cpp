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

#include "vitoconnect_datapoint.h"

namespace esphome {
namespace vitoconnect {

std::function<void(uint8_t[], uint8_t, Datapoint* dp)> Datapoint::_stdOnData = nullptr;

Datapoint::Datapoint() : _address(0), _length(0) {
  // empty
}

Datapoint::~Datapoint() {
  // empty
}

void Datapoint::onData(std::function<void(uint8_t[], uint8_t, Datapoint* dp)> callback) {
  _stdOnData = callback;
}

void Datapoint::encode(uint8_t* raw, uint8_t length) {
  if (raw == nullptr || length == 0) return;
  memset(raw, 0, length);
}

void Datapoint::encode(uint8_t* raw, uint8_t length, void* data) {
  if (raw == nullptr || length == 0) return;
  memset(raw, 0, length);
  if (data == nullptr) return;
  const uint8_t n = (length < _length) ? length : _length;
  if (n > 0) memcpy(raw, data, n);
}

void Datapoint::decode(uint8_t* data, uint8_t length, Datapoint* dp) {
  if (data == nullptr) return;
  if (length != _length) return;
  if (_stdOnData) _stdOnData(data, length, dp);
}

}  // namespace vitoconnect
}  // namespace esphome
