// Standalone host-side test for the Optolink number codec.
//
// This file duplicates the sign-extension / little-endian decode and encode
// math from components/vitoconnect/number/vitoconnect_number.cpp as free
// functions so the algorithm can be exercised on a normal host without any
// ESP-IDF / ESPHome dependencies.
//
// Build:  g++ -std=c++17 -Wall -Wextra test/test_number_algorithm.cpp -o test_number_algorithm
// Run:    ./test_number_algorithm   (exit 0 == all tests passed)

#include <cmath>
#include <cstdint>
#include <cstdio>

// ---------------------------------------------------------------------------
// Reimplementation of OPTOLINKNumber::decode (integer part).
//
// Mirrors vitoconnect_number.cpp::decode():
//   - build a little-endian unsigned integer from `length` bytes
//   - if is_signed, sign-extend via the `u | (~mask)` trick
//   - if unsigned, clamp 8-byte values above INT64_MAX
// ---------------------------------------------------------------------------
int64_t decode_number(const uint8_t* data, uint8_t length, bool is_signed) {
  uint64_t u = 0;
  for (uint8_t i = 0; i < length; i++) {
    u |= (uint64_t) data[i] << (8 * i);
  }

  int64_t iv = 0;
  if (is_signed) {
    const uint8_t bits = length * 8;
    const uint64_t sign_bit = 1ULL << (bits - 1);
    const uint64_t mask = (bits == 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << bits) - 1ULL);

    if (u & sign_bit) {
      iv = (int64_t) (u | (~mask));
    } else {
      iv = (int64_t) u;
    }
  } else {
    const uint64_t max_i64 = static_cast<uint64_t>(INT64_MAX);
    if (length == 8 && u > max_i64) {
      u = max_i64;
    }
    iv = (int64_t) u;
  }

  return iv;
}

// ---------------------------------------------------------------------------
// Reimplementation of OPTOLINKNumber::encode (float part).
//
// Mirrors vitoconnect_number.cpp::encode():
//   - scale by div_ratio, round to nearest integer
//   - clamp to the representable range for (length, sign)
//   - write little-endian bytes
// `div_ratio` defaults to 1.0 so callers may omit it.
// ---------------------------------------------------------------------------
void encode_number(uint8_t* raw, uint8_t length, bool is_signed,
                   double value, float div_ratio = 1.0f) {
  for (uint8_t i = 0; i < length; i++) {
    raw[i] = 0;
  }

  if (div_ratio <= 0.0f) {
    div_ratio = 1.0f;
  }

  const uint8_t bits = length * 8;
  const double scaled = value * (double) div_ratio;
  int64_t iv = (int64_t) llround(scaled);

  int64_t min_v = 0;
  int64_t max_v = 0;
  uint64_t mask = 0;

  if (is_signed) {
    if (bits == 64) {
      min_v = INT64_MIN;
      max_v = INT64_MAX;
      mask = 0xFFFFFFFFFFFFFFFFULL;
    } else {
      min_v = -(1LL << (bits - 1));
      max_v = (1LL << (bits - 1)) - 1;
      mask = (1ULL << bits) - 1ULL;
    }
  } else {
    min_v = 0;
    const uint64_t max_u = (bits == 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << bits) - 1ULL);
    max_v = (bits == 64) ? INT64_MAX : (int64_t) max_u;
    mask = max_u;
  }

  if (iv < min_v) {
    iv = min_v;
  } else if (iv > max_v) {
    iv = max_v;
  }

  uint64_t u = 0;
  if (is_signed) {
    u = ((uint64_t) iv) & mask;
  } else {
    u = (uint64_t) iv;
  }

  for (uint8_t i = 0; i < length; i++) {
    raw[i] = (uint8_t) ((u >> (8 * i)) & 0xFF);
  }
}

// ---------------------------------------------------------------------------
// Tiny test harness.
// ---------------------------------------------------------------------------
static int g_failures = 0;

static void check(bool cond, const char* what) {
  if (cond) {
    std::printf("PASS: %s\n", what);
  } else {
    std::printf("FAIL: %s\n", what);
    g_failures++;
  }
}

static bool almost_equal(double a, double b, double tol = 1e-4) {
  return std::fabs(a - b) < tol;
}

int main() {
  // Signed 1-byte.
  {
    const uint8_t raw1[] = {0xFF};
    check(decode_number(raw1, 1, true) == -1, "signed 1-byte [0xFF] == -1");

    const uint8_t raw2[] = {0x80};
    check(decode_number(raw2, 1, true) == -128, "signed 1-byte [0x80] == -128");

    const uint8_t raw3[] = {0x7F};
    check(decode_number(raw3, 1, true) == 127, "signed 1-byte [0x7F] == 127");
  }

  // Signed 2-byte.
  {
    const uint8_t raw1[] = {0xFF, 0xFF};
    check(decode_number(raw1, 2, true) == -1, "signed 2-byte [0xFF,0xFF] == -1");

    const uint8_t raw2[] = {0x00, 0x80};
    check(decode_number(raw2, 2, true) == -32768, "signed 2-byte [0x00,0x80] == -32768");

    const uint8_t raw3[] = {0x10, 0x00};
    check(decode_number(raw3, 2, true) == 16, "signed 2-byte [0x10,0x00] == 16");
  }

  // Signed 4-byte.
  {
    const uint8_t raw1[] = {0xFF, 0xFF, 0xFF, 0xFF};
    check(decode_number(raw1, 4, true) == -1, "signed 4-byte [0xFF x4] == -1");

    const uint8_t raw2[] = {0x00, 0x00, 0x00, 0x80};
    check(decode_number(raw2, 4, true) == -((1LL << 31)),
          "signed 4-byte [0x00,0x00,0x00,0x80] == INT32_MIN");

    const uint8_t raw3[] = {0x78, 0x56, 0x34, 0x12};
    check(decode_number(raw3, 4, true) == 0x12345678,
          "signed 4-byte [0x78,0x56,0x34,0x12] == 0x12345678");
  }

  // Unsigned 1-byte.
  {
    const uint8_t raw1[] = {0x80};
    check(decode_number(raw1, 1, false) == 128, "unsigned 1-byte [0x80] == 128");

    const uint8_t raw2[] = {0x7F};
    check(decode_number(raw2, 1, false) == 127, "unsigned 1-byte [0x7F] == 127");

    const uint8_t raw3[] = {0xFF};
    check(decode_number(raw3, 1, false) == 255, "unsigned 1-byte [0xFF] == 255");
  }

  // div_ratio with sign extension -> float result.
  {
    const uint8_t raw[] = {0x7B};
    const int64_t iv = decode_number(raw, 1, true);
    const double value = (double) iv / 10.0;
    check(almost_equal(value, 12.3), "div_ratio=10.0 raw=123 signed 1-byte -> 12.3");
  }

  // Negative value with div_ratio (sign extension must survive scaling).
  {
    const uint8_t raw[] = {0xFF};
    const int64_t iv = decode_number(raw, 1, true);
    const double value = (double) iv / 10.0;
    check(almost_equal(value, -0.1), "div_ratio=10.0 raw=[0xFF] signed 1-byte -> -0.1");
  }

  // Encode / decode round trip (signed, with div_ratio).
  {
    uint8_t raw[4] = {0, 0, 0, 0};
    encode_number(raw, 4, true, 12.3, 10.0f);
    const int64_t iv = decode_number(raw, 4, true);
    const double value = (double) iv / 10.0;
    check(almost_equal(value, 12.3), "encode/decode round trip 12.3 (div_ratio=10) == 12.3");
  }

  // Encode unsigned clamps to representable range.
  {
    uint8_t raw[1] = {0};
    encode_number(raw, 1, false, 300.0, 1.0f);
    check(raw[0] == 255, "unsigned 1-byte clamp of 300.0 -> 255");
  }

  std::printf("\n%d check(s) failed\n", g_failures);
  if (g_failures == 0) {
    std::printf("ALL TESTS PASSED\n");
    return 0;
  }
  std::printf("TESTS FAILED\n");
  return 1;
}
