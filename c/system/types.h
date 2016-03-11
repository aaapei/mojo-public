// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains types and constants/macros common to different Mojo system
// APIs.
// TODO(vtl): Split this file apart (and eliminate).
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_TYPES_H_
#define MOJO_PUBLIC_C_SYSTEM_TYPES_H_

#include <stdint.h>

// TODO(vtl): These includes are here only for historic reasons. If you actually
// need either, please include it directly.
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"

// TODO(vtl): Notes: Use of undefined flags will lead to undefined behavior
// (typically they'll be ignored), not necessarily an error.

// |MojoTimeTicks|: A time delta, in microseconds, the meaning of which is
// source-dependent.

typedef int64_t MojoTimeTicks;

// |MojoDeadline|: Used to specify deadlines (timeouts), in microseconds (except
// for |MOJO_DEADLINE_INDEFINITE|).
//   |MOJO_DEADLINE_INDEFINITE| - Used to indicate "forever".

typedef uint64_t MojoDeadline;

#define MOJO_DEADLINE_INDEFINITE ((MojoDeadline)-1)

#endif  // MOJO_PUBLIC_C_SYSTEM_TYPES_H_
