// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains Mojo system handle-related declarations/definitions.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_HANDLE_H_
#define MOJO_PUBLIC_C_SYSTEM_HANDLE_H_

#include <stdint.h>

#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/result.h"

// |MojoHandle|: Handles to Mojo objects.
//   |MOJO_HANDLE_INVALID| - A value that is never a valid handle.

typedef uint32_t MojoHandle;

#define MOJO_HANDLE_INVALID ((MojoHandle)0)

// |MojoHandleSignals|: Used to specify signals that can be waited on for a
// handle (and which can be triggered), e.g., the ability to read or write to
// the handle.
//   |MOJO_HANDLE_SIGNAL_NONE| - No flags. |MojoWait()|, etc. will return
//       |MOJO_RESULT_FAILED_PRECONDITION| if you attempt to wait on this.
//   |MOJO_HANDLE_SIGNAL_READABLE| - Can read (e.g., a message) from the handle.
//   |MOJO_HANDLE_SIGNAL_WRITABLE| - Can write (e.g., a message) to the handle.
//   |MOJO_HANDLE_SIGNAL_PEER_CLOSED| - The peer handle is closed.

typedef uint32_t MojoHandleSignals;

#define MOJO_HANDLE_SIGNAL_NONE ((MojoHandleSignals)0)
#define MOJO_HANDLE_SIGNAL_READABLE ((MojoHandleSignals)1 << 0)
#define MOJO_HANDLE_SIGNAL_WRITABLE ((MojoHandleSignals)1 << 1)
#define MOJO_HANDLE_SIGNAL_PEER_CLOSED ((MojoHandleSignals)1 << 2)

// |MojoHandleSignalsState|: Returned by wait functions to indicate the
// signaling state of handles. Members are as follows:
//   - |satisfied signals|: Bitmask of signals that were satisfied at some time
//         before the call returned.
//   - |satisfiable signals|: These are the signals that are possible to
//         satisfy. For example, if the return value was
//         |MOJO_RESULT_FAILED_PRECONDITION|, you can use this field to
//         determine which, if any, of the signals can still be satisfied.
// Note: This struct is not extensible (and only has 32-bit quantities), so it's
// 32-bit-aligned.
MOJO_STATIC_ASSERT(MOJO_ALIGNOF(uint32_t) == 4, "uint32_t has weird alignment");
struct MOJO_ALIGNAS(4) MojoHandleSignalsState {
  MojoHandleSignals satisfied_signals;
  MojoHandleSignals satisfiable_signals;
};
MOJO_STATIC_ASSERT(sizeof(MojoHandleSignalsState) == 8,
                   "MojoHandleSignalsState has wrong size");

#ifdef __cplusplus
extern "C" {
#endif

// Closes the given |handle|.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |handle| is not a valid handle.
//
// Concurrent operations on |handle| may succeed (or fail as usual) if they
// happen before the close, be cancelled with result |MOJO_RESULT_CANCELLED| if
// they properly overlap (this is likely the case with |MojoWait()|, etc.), or
// fail with |MOJO_RESULT_INVALID_ARGUMENT| if they happen after.
MojoResult MojoClose(MojoHandle handle);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MOJO_PUBLIC_C_SYSTEM_HANDLE_H_
