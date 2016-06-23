// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/array.h"

#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Tests MojomArray_New().
TEST(ArrayTest, New) {
  char bytes_buffer[1000];
  struct MojomBuffer buf = {bytes_buffer, sizeof(bytes_buffer), 0};

  struct MojomArrayHeader* arr = NULL;
  arr = MojomArray_New(&buf, 3, sizeof(uint32_t));
  EXPECT_EQ(buf.buf, (char*)arr);

  // Test that things are rounded up if data is not already multiple-of-8.
  EXPECT_EQ(8u                          // array header
                + 3 * sizeof(uint32_t)  // space for 3 uint32_ts.
                + 4u,                   // padding to round up to 8 bytes.
            buf.num_bytes_used);

  EXPECT_EQ(buf.num_bytes_used, arr->num_bytes);
  EXPECT_EQ(3ul, arr->num_elements);

  // Test failure when we try to allocate too much.
  EXPECT_EQ(NULL, MojomArray_New(&buf, UINT32_MAX, sizeof(uint32_t)));
  EXPECT_EQ(NULL, MojomArray_New(&buf, 1000, sizeof(uint32_t)));

  // Test the simple case (no rounding necessary).
  buf.num_bytes_used = 0;
  arr = MojomArray_New(&buf, 4, sizeof(uint32_t));
  EXPECT_EQ(8u                           // array header
                + 4 * sizeof(uint32_t),  // space for 4 uint32_ts
            buf.num_bytes_used);
  EXPECT_EQ(buf.num_bytes_used, arr->num_bytes);
  EXPECT_EQ(4ul, arr->num_elements);
}

}  // namespace
