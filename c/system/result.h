// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the definition of |MojoResult| and related macros.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_RESULT_H_
#define MOJO_PUBLIC_C_SYSTEM_RESULT_H_

#include <stdint.h>

// |MojoResult|: Result codes for Mojo operations. The only success code is zero
// (|MOJO_RESULT_OK|); all non-zero values should be considered as error/failure
// codes (even if the value is not recognized).
//   |MOJO_RESULT_OK| - Not an error; returned on success.
//   |MOJO_RESULT_CANCELLED| - Operation was cancelled, typically by the caller.
//   |MOJO_RESULT_UNKNOWN| - Unknown error (e.g., if not enough information is
//       available for a more specific error).
//   |MOJO_RESULT_INVALID_ARGUMENT| - Caller specified an invalid argument. This
//       differs from |MOJO_RESULT_FAILED_PRECONDITION| in that the former
//       indicates arguments that are invalid regardless of the state of the
//       system.
//   |MOJO_RESULT_DEADLINE_EXCEEDED| - Deadline expired before the operation
//       could complete.
//   |MOJO_RESULT_NOT_FOUND| - Some requested entity was not found (i.e., does
//       not exist).
//   |MOJO_RESULT_ALREADY_EXISTS| - Some entity or condition that we attempted
//       to create already exists.
//   |MOJO_RESULT_PERMISSION_DENIED| - The caller does not have permission to
//       for the operation (use |MOJO_RESULT_RESOURCE_EXHAUSTED| for rejections
//       caused by exhausting some resource instead).
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| - Some resource required for the call
//       (possibly some quota) has been exhausted.
//   |MOJO_RESULT_FAILED_PRECONDITION| - The system is not in a state required
//       for the operation (use this if the caller must do something to rectify
//       the state before retrying).
//   |MOJO_RESULT_ABORTED| - The operation was aborted by the system, possibly
//       due to a concurrency issue (use this if the caller may retry at a
//       higher level).
//   |MOJO_RESULT_OUT_OF_RANGE| - The operation was attempted past the valid
//       range. Unlike |MOJO_RESULT_INVALID_ARGUMENT|, this indicates that the
//       operation may be/become valid depending on the system state. (This
//       error is similar to |MOJO_RESULT_FAILED_PRECONDITION|, but is more
//       specific.)
//   |MOJO_RESULT_UNIMPLEMENTED| - The operation is not implemented, supported,
//       or enabled.
//   |MOJO_RESULT_INTERNAL| - Internal error: this should never happen and
//       indicates that some invariant expected by the system has been broken.
//   |MOJO_RESULT_UNAVAILABLE| - The operation is (temporarily) currently
//       unavailable. The caller may simply retry the operation (possibly with a
//       backoff).
//   |MOJO_RESULT_DATA_LOSS| - Unrecoverable data loss or corruption.
//   |MOJO_RESULT_BUSY| - One of the resources involved is currently being used
//       (possibly on another thread) in a way that prevents the current
//       operation from proceeding, e.g., if the other operation may result in
//       the resource being invalidated.
//   |MOJO_RESULT_SHOULD_WAIT| - The request cannot currently be completed
//       (e.g., if the data requested is not yet available). The caller should
//       wait for it to be feasible using |MojoWait()| or |MojoWaitMany()|.
//
// The codes from |MOJO_RESULT_OK| to |MOJO_RESULT_DATA_LOSS| come from
// Google3's canonical error codes.
//
// TODO(vtl): Add a |MOJO_RESULT_UNSATISFIABLE|?

typedef uint32_t MojoResult;

#define MOJO_RESULT_OK ((MojoResult)0)
#define MOJO_RESULT_CANCELLED ((MojoResult)1)
#define MOJO_RESULT_UNKNOWN ((MojoResult)2)
#define MOJO_RESULT_INVALID_ARGUMENT ((MojoResult)3)
#define MOJO_RESULT_DEADLINE_EXCEEDED ((MojoResult)4)
#define MOJO_RESULT_NOT_FOUND ((MojoResult)5)
#define MOJO_RESULT_ALREADY_EXISTS ((MojoResult)6)
#define MOJO_RESULT_PERMISSION_DENIED ((MojoResult)7)
#define MOJO_RESULT_RESOURCE_EXHAUSTED ((MojoResult)8)
#define MOJO_RESULT_FAILED_PRECONDITION ((MojoResult)9)
#define MOJO_RESULT_ABORTED ((MojoResult)10)
#define MOJO_RESULT_OUT_OF_RANGE ((MojoResult)11)
#define MOJO_RESULT_UNIMPLEMENTED ((MojoResult)12)
#define MOJO_RESULT_INTERNAL ((MojoResult)13)
#define MOJO_RESULT_UNAVAILABLE ((MojoResult)14)
#define MOJO_RESULT_DATA_LOSS ((MojoResult)15)
#define MOJO_RESULT_BUSY ((MojoResult)16)
#define MOJO_RESULT_SHOULD_WAIT ((MojoResult)17)

#endif  // MOJO_PUBLIC_C_SYSTEM_TYPES_H_
