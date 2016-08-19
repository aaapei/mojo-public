// Copyright 2016 The Fucshia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains types/constants and functions specific to events.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_INCLUDE_MOJO_SYSTEM_EVENT_H_
#define MOJO_PUBLIC_C_INCLUDE_MOJO_SYSTEM_EVENT_H_

#include <mojo/macros.h>
#include <mojo/system/handle.h>
#include <mojo/system/result.h>

// |MojoCreateEventOptions|: Used to specify creation parameters for an event to
// |MojoCreateEvent()|.
//   |uint32_t struct_size|: Set to the size of the
//       |MojoCreateEventOptions| struct. (Used to allow for future extensions.)
//   |MojoCreateEventOptionsFlags flags|: Reserved for future use.
//       |MOJO_CREATE_EVENT_OPTIONS_FLAG_NONE|: No flags; default mode.

typedef uint32_t MojoCreateEventOptionsFlags;

#define MOJO_CREATE_MESSAGE_PIPE_OPTIONS_FLAG_NONE \
  ((MojoCreateEventOptionsFlags)0)

MOJO_STATIC_ASSERT(MOJO_ALIGNOF(int64_t) == 8, "int64_t has weird alignment");
struct MOJO_ALIGNAS(8) MojoCreateEventOptions {
  uint32_t struct_size;
  MojoCreateEventOptionsFlags flags;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoCreateEventOptions) == 8,
                   "MojoCreateEventOptions has wrong size");

MOJO_BEGIN_EXTERN_C

// |MojoCreateEvent()|: Creates an event, which is an object with
// user-controllable handle signals, |MOJO_HANDLE_SIGNAL_SIGNAL[0-4]|.
//
// |options| may be set to null for an event with the default options.
//
// On success, |*event_handle| is set to a handle for the event. The handle has
// (at least) the following rights: |MOJO_HANDLE_RIGHT_DUPLICATE|,
// |MOJO_HANDLE_RIGHT_TRANSFER|, |MOJO_HANDLE_RIGHT_READ|,
// |MOJO_HANDLE_RIGHT_WRITE|, |MOJO_HANDLE_RIGHT_GET_OPTIONS|, and
// |MOJO_HANDLE_RIGHT_SET_OPTIONS|.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_SYSTEM_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |*options| is invalid).
//   |MOJO_SYSTEM_RESULT_RESOURCE_EXHAUSTED| if a process/system/quota/etc.
//       limit has been reached.
//   |MOJO_SYSTEM_RESULT_UNIMPLEMENTED| if an unsupported flag was set in
//       |*options|.
MojoResult MojoCreateEvent(
    const struct MojoCreateEventOptions* MOJO_RESTRICT options,  // Optional in.
    MojoHandle* MOJO_RESTRICT event_handle);                     // Out.

// |MojoEventSignal()|: Sets/clears the signals of an event.
//
// First, the handle signals specified by |clear_signals| are cleared. Then the
// signals specified by |set_signals| are set.
//
// The set of signals specified by |clear_signals| and |set_signals| must be in
// the set |MOJO_HANDLE_SIGNAL_SIGNAL[0-4]| (i.e., those under the mask
// |MOJO_HANDLE_SIGNAL_SIGNALS_MASK|). The two sets of signals may "overlap"
// and, as indicated above, signals are cleared before they are set.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_SYSTEM_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       if |event_handle| is not a valid handle, or |clear_signals| or
//       |set_signals| specify invalid signals.
//   |MOJO_SYSTEM_RESULT_PERMISSION_DENIED| if |event_handle| does not have the
//       |MOJO_HANDLE_RIGHT_WRITE| right.
MojoResult MojoEventSignal(MojoHandle event_handle,          // In.
                           MojoHandleSignals clear_signals,  // In.
                           MojoHandleSignals set_signals);   // In.

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_INCLUDE_MOJO_SYSTEM_EVENT_H_
