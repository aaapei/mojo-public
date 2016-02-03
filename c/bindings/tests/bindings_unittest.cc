// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <string>
#include <vector>

#include "mojo/public/c/bindings/message.h"
#include "mojo/public/c/bindings/struct.h"
#include "mojo/public/cpp/bindings/tests/validation_test_input_parser.h"
#include "mojo/public/cpp/system/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(CBindingsTest, InvalidStructHeader) {
  struct TestCase {
    const char* test_case;
    size_t size;
    const char* name;
  };

  TestCase cases[] = {
      {"", 0u, "zero size"},
      {"", 4u, "Header too small"},
      {"[u4]1  // num_bytes\n"
       "[u4]0  // version",
       16u, "num_bytes too small"},
      {"[u4]20 // num_bytes\n"
       "[u4]0  // version",
       16u, "num_bytes bigger than buffer"},
  };
  for (size_t i = 0u; i < MOJO_ARRAYSIZE(cases); ++i) {
    std::vector<uint8_t> data;
    std::string parser_error_message;
    size_t num_handles = 0u;
    ASSERT_TRUE(mojo::test::ParseValidationTestInput(
        cases[i].test_case, &data, &num_handles, &parser_error_message))
        << parser_error_message;
    EXPECT_FALSE(mojo_validate_struct_header(data.data(), cases[i].size))
        << cases[i].name;
  }
}

TEST(CBindingsTest, ValidStructHeader) {
  struct TestCase {
    const char* test_case;
    size_t size;
    uint32_t expected_num_bytes;
    uint32_t expected_version;
  };

  TestCase cases[] = {
      {"[u4]16 [u4]0", 16u, 16u, 0u},
      {"[u4]40 [u4]0", 40u, 40u, 0u},
      {"[u4]16 [u4]13", 16u, 16u, 13u},
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version",
       20u, 16u, 0u},
  };

  for (size_t i = 0u; i < MOJO_ARRAYSIZE(cases); ++i) {
    std::vector<uint8_t> data;
    std::string parser_error_message;
    size_t num_handles = 0u;
    ASSERT_TRUE(mojo::test::ParseValidationTestInput(
        cases[i].test_case, &data, &num_handles, &parser_error_message))
        << parser_error_message << " case " << i;
    EXPECT_TRUE(mojo_validate_struct_header(data.data(), cases[i].size))
        << " case " << i;
    mojo_struct_header_t* header =
        reinterpret_cast<mojo_struct_header_t*>(data.data());
    EXPECT_EQ(header->num_bytes, cases[i].expected_num_bytes) << " case " << i;
    EXPECT_EQ(header->version, cases[i].expected_version) << " case " << i;
  }
}

TEST(CBindingsTest, InvalidMessageHeader) {
  struct TestCase {
    const char* test_case;
    size_t size;
    const char* name;
  };

  TestCase cases[] = {
      {"[u4]8  // num_bytes\n"
       "[u4]0  // version",
       16u, "num_bytes too small"},
      {"[u4]24 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]0  // flags\n"
       "[u8]0  // request id",
       24u, "version 0 header with version 1 size"},
      {"[u4]16 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]0  // flags",
       20u, "version 1 header with version 0 size"},
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]1  // flags",
       16u, "version 0 header with expect response flag"},
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]2  // flags",
       16u, "version 0 header with is response flag"},
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]3  // flags",
       16u, "version 0 header with both is/expects response flags"},
      {"[u4]24 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]3  // flags\n"
       "[u8]0  // request id",
       24u, "version 1 header with both is/expects response flags"},
  };
  for (size_t i = 0u; i < MOJO_ARRAYSIZE(cases); ++i) {
    std::vector<uint8_t> data;
    std::string parser_error_message;
    size_t num_handles = 0u;
    ASSERT_TRUE(mojo::test::ParseValidationTestInput(
        cases[i].test_case, &data, &num_handles, &parser_error_message))
        << parser_error_message << " case " << i;
    ASSERT_TRUE(mojo_validate_struct_header(data.data(), cases[i].size))
        << " case " << i;
    mojo_struct_header_t* header =
        reinterpret_cast<mojo_struct_header_t*>(data.data());
    EXPECT_FALSE(mojo_validate_message_header(header, cases[i].size))
        << cases[i].name << " case " << i;
  }
}

TEST(CBindingsTest, ValidMessageHeader) {
  struct TestCase {
    const char* test_case;
    size_t size;
  };

  TestCase cases[] = {
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]0  // flags",
       16u},  // version 0 header
      {"[u4]24 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]1  // flags\n"
       "[u8]0  // request_id",
       24u},  // version 1 request
      {"[u4]24 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]2  // flags\n"
       "[u8]0  // request_id",
       24u},  // version 1 response
      {"[u4]24 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]0  // flags\n"
       "[u8]0  // request id",
       24u},  // version 1 header without is/expects response flags
  };

  for (size_t i = 0u; i < MOJO_ARRAYSIZE(cases); ++i) {
    std::vector<uint8_t> data;
    std::string parser_error_message;
    size_t num_handles = 0u;
    ASSERT_TRUE(mojo::test::ParseValidationTestInput(
        cases[i].test_case, &data, &num_handles, &parser_error_message))
        << parser_error_message << " case " << i;
    ASSERT_TRUE(mojo_validate_struct_header(data.data(), cases[i].size))
        << " case " << i;
    mojo_struct_header_t* header =
        reinterpret_cast<mojo_struct_header_t*>(data.data());
    EXPECT_TRUE(mojo_validate_message_header(header, cases[i].size)) << " case "
                                                                     << i;
  }
}

}  // namespace
