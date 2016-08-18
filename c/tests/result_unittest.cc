// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C result API (the macros declared in
// mojo/public/c/include/mojo/result.h).

#include <mojo/result.h>

#include "gtest/gtest.h"

namespace {

TEST(ResultTest, Macros) {
  EXPECT_EQ(0x12345678u, MOJO_RESULT_MAKE(static_cast<MojoResult>(0x8u),
                                          static_cast<MojoResult>(0x1234u),
                                          static_cast<MojoResult>(0x567u)));
  EXPECT_EQ(0x8u, MOJO_RESULT_GET_CODE(static_cast<MojoResult>(0x12345678u)));
  EXPECT_EQ(0x1234u,
            MOJO_RESULT_GET_SPACE(static_cast<MojoResult>(0x12345678u)));
  EXPECT_EQ(0x567u,
            MOJO_RESULT_GET_SUBCODE(static_cast<MojoResult>(0x12345678u)));

  EXPECT_EQ(0xfedcfedfu, MOJO_RESULT_MAKE(static_cast<MojoResult>(0xfu),
                                          static_cast<MojoResult>(0xfedcu),
                                          static_cast<MojoResult>(0xfedu)));
  EXPECT_EQ(0xfu, MOJO_RESULT_GET_CODE(static_cast<MojoResult>(0xfedcfedfu)));
  EXPECT_EQ(0xfedcu,
            MOJO_RESULT_GET_SPACE(static_cast<MojoResult>(0xfedcfedfu)));
  EXPECT_EQ(0xfedu,
            MOJO_RESULT_GET_SUBCODE(static_cast<MojoResult>(0xfedcfedfu)));
}

TEST(ResultTest, ResultMacros) {
  EXPECT_EQ(0x00000000u, MOJO_RESULT_OK);
}

}  // namespace
