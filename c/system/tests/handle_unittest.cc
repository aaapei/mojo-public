// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C handle API (the functions declared in
// mojo/public/c/system/handle.h). Note: The functionality of these APIs for
// specific types of handles are tested with the APIs for those types of
// handles.

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(HandleTest, InvalidHandle) {
  // Close:
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(MOJO_HANDLE_INVALID));

  // GetRights:
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoGetRights(MOJO_HANDLE_INVALID, &rights));

  // DuplicateHandleWithReducedRights:
  MojoHandle new_handle = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoDuplicateHandleWithReducedRights(
                MOJO_HANDLE_INVALID, MOJO_HANDLE_RIGHT_DUPLICATE, &new_handle));
  EXPECT_EQ(MOJO_HANDLE_INVALID, new_handle);

  // DuplicateHandle:
  new_handle = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoDuplicateHandle(MOJO_HANDLE_INVALID, &new_handle));
  EXPECT_EQ(MOJO_HANDLE_INVALID, new_handle);
}

}  // namespace
