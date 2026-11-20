#include <gtest/gtest.h>
#include <cstring>
#include <iomanip>
#include "au_parser.h"
#include "PMUTimer.h"

using namespace WelsDec;

namespace {

#define BUF_SIZE 4096
#define PADDING_SIZE 64

void TestThreeByteStartcode(PDetectStartCodePrefixFunc func, const char* impl_name) {
  uint8_t buf[BUF_SIZE + PADDING_SIZE];
  int32_t offset;

  for (int i = 0; i < 100; i++) {
    memset(buf, 0xFF, BUF_SIZE);
    buf[i] = 0x00;
    buf[i + 1] = 0x00;
    buf[i + 2] = 0x01;

    uint8_t* result = func(buf, &offset, BUF_SIZE);
    ASSERT_NE(result, nullptr) << impl_name << ": Failed at offset " << i;
    EXPECT_EQ(offset, i + 3) << impl_name << ": Offset mismatch at position " << i;
  }
}

void TestLeadingZerosPatterns(PDetectStartCodePrefixFunc func, const char* impl_name) {
  uint8_t buf[BUF_SIZE + PADDING_SIZE];
  int32_t offset;

  for (int leading_zeros = 2; leading_zeros <= 10; leading_zeros++) {
    for (int i = 0; i < 50; i++) {
      memset(buf, 0xFF, BUF_SIZE);

      for (int z = 0; z < leading_zeros; z++) {
        buf[i + z] = 0x00;
      }
      buf[i + leading_zeros] = 0x01;

      uint8_t* result = func(buf, &offset, BUF_SIZE);
      ASSERT_NE(result, nullptr) << impl_name << ": Failed at offset " << i << " with " << leading_zeros << " leading zeros";
      EXPECT_EQ(offset, i + leading_zeros + 1) << impl_name << ": Offset mismatch at position " << i << " with " << leading_zeros << " leading zeros";
    }
  }
}

void TestNoStartcodePatterns(PDetectStartCodePrefixFunc func, const char* impl_name) {
  uint8_t buf[BUF_SIZE + PADDING_SIZE];
  int32_t offset;

  // Pattern 1: All zeros except no 0x01
  memset(buf, 0x00, BUF_SIZE);
  uint8_t* result = func(buf, &offset, BUF_SIZE);
  EXPECT_EQ(result, nullptr) << impl_name << ": Should not find startcode in all zeros";

  // Pattern 2: 0x00 0x00 0x02 (wrong last byte)
  for (int i = 0; i < 50; i++) {
    memset(buf, 0xFF, BUF_SIZE);
    buf[i] = 0x00;
    buf[i + 1] = 0x00;
    buf[i + 2] = 0x02;

    result = func(buf, &offset, BUF_SIZE);
    EXPECT_EQ(result, nullptr) << impl_name << ": Should not find startcode at offset " << i;
  }
}

void TestSmallBuffers(PDetectStartCodePrefixFunc func, const char* impl_name) {
  uint8_t buf[16];
  int32_t offset;

  for (int size = 4; size <= 16; size++) {
    memset(buf, 0xFF, 16);
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x01;

    uint8_t* result = func(buf, &offset, size);
    ASSERT_NE(result, nullptr) << impl_name << ": Failed for buffer size " << size;
    EXPECT_EQ(offset, 4) << impl_name << ": Offset mismatch for buffer size " << size;
  }
}

void TestMultipleStartcodes(PDetectStartCodePrefixFunc func, const char* impl_name) {
  uint8_t buf[BUF_SIZE + PADDING_SIZE];
  int32_t offset;

  for (int first_pos = 10; first_pos < 50; first_pos += 10) {
    for (int second_pos = first_pos + 10; second_pos < first_pos + 50; second_pos += 10) {
      memset(buf, 0xFF, BUF_SIZE);

      buf[first_pos] = 0x00;
      buf[first_pos + 1] = 0x00;
      buf[first_pos + 2] = 0x00;
      buf[first_pos + 3] = 0x01;

      buf[second_pos] = 0x00;
      buf[second_pos + 1] = 0x00;
      buf[second_pos + 2] = 0x01;

      uint8_t* result = func(buf, &offset, BUF_SIZE);
      ASSERT_NE(result, nullptr) << impl_name << ": Failed at first_pos=" << first_pos << ", second_pos=" << second_pos;
      EXPECT_EQ(offset, first_pos + 4) << impl_name << ": Should find first startcode at " << first_pos;
    }
  }
}

void TestPerformance(PDetectStartCodePrefixFunc func, const char* impl_name) {
  const int buf_size = 4096;
  uint8_t buf[buf_size];
  int32_t offset;
  uint64_t total_cycles = 0;

  // Performance test with startcode at different positions
  PMUTimer timer_cycles(PMUTimer::CPU_CYCLES);
  for (int i = 0; i < buf_size - 4; i++) {
    memset(buf, 0x2, buf_size);
    buf[i] = 0x00;
    buf[i + 1] = 0x00;
    buf[i + 2] = 0x00;
    buf[i + 3] = 0x01;
    timer_cycles.start();
    func(buf, &offset, buf_size);
    timer_cycles.stop();
    total_cycles += timer_cycles.read();
  }

  double cycles_per_iter = static_cast<double>(total_cycles) / (buf_size - 4);
  std::cout << std::endl;
  std::cout << "=== Performance Test(" << impl_name << ") ===" << std::endl;
  std::cout << "Cycles per iteration: " << std::fixed << std::setprecision(2) << cycles_per_iter << std::endl;
}

// Macro to run tests for different implementations
#define RUN_DETECT_STARTCODE_TESTS(impl_func, impl_name) \
  TestThreeByteStartcode(impl_func, impl_name); \
  TestLeadingZerosPatterns(impl_func, impl_name); \
  TestNoStartcodePatterns(impl_func, impl_name); \
  TestSmallBuffers(impl_func, impl_name); \
  TestMultipleStartcodes(impl_func, impl_name)

// Test C implementation
TEST(DetectStartCodePrefixTest, C) {
  RUN_DETECT_STARTCODE_TESTS(DetectStartCodePrefixC, "C");
}

#if defined(HAVE_NEON_AARCH64) && defined(__aarch64__)
// Test NEON implementation
TEST(DetectStartCodePrefixTest, NEON) {
  RUN_DETECT_STARTCODE_TESTS(DetectStartCodePrefixNEON, "NEON");
}

// Test performance comparison between C and NEON
TEST(DetectStartCodePrefixTest, Performance) {
  TestPerformance(DetectStartCodePrefixC, "C");
  TestPerformance(DetectStartCodePrefixNEON, "NEON");
}
#endif

}  // namespace