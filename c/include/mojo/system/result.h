// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains |MojoResult| definitions for the system error space
// (|MOJO_ERROR_SPACE_SYSTEM|).
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_INCLUDE_MOJO_SYSTEM_RESULT_H_
#define MOJO_PUBLIC_C_INCLUDE_MOJO_SYSTEM_RESULT_H_

#include <mojo/result.h>

// System error space error subcodes -------------------------------------------

// Subcodes valid for |MOJO_ERROR_CODE_FAILED_PRECONDITION|:
//   |MOJO_ERROR_CODE_FAILED_PRECONDITION_SUBCODE_BUSY| - One of the resources
//       involved is currently being used (possibly on another thread) in a way
//       that prevents the current operation from proceeding, e.g., if the other
//       operation may result in the resource being invalidated. TODO(vtl): We
//       should probably get rid of this, and report "invalid argument" instead
//       (with a different subcode scheme). This is here now for ease of
//       conversion with the existing |MOJO_RESULT_BUSY|.
#define MOJO_SYSTEM_ERROR_CODE_FAILED_PRECONDITION_SUBCODE_BUSY \
  ((MojoResult)0x001u)

// Subcodes valid for |MOJO_ERROR_CODE_UNAVAILABLE|:
//   |MOJO_SYSTEM_ERROR_CODE_UNAVAILABLE_SUBCODE_SHOULD_WAIT| - The request
//       cannot currently be completed (e.g., if the data requested is not yet
//       available). The caller should wait for it to be feasible using one of
//       the wait primitives.
#define MOJO_SYSTEM_ERROR_CODE_UNAVAILABLE_SUBCODE_SHOULD_WAIT \
  ((MojoResult)0x001u)

// Complete results ------------------------------------------------------------

// Error code |MOJO_ERROR_CODE_CANCELLED|:
#define MOJO_SYSTEM_RESULT_CANCELLED                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_CANCELLED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_UNKNOWN|:
#define MOJO_SYSTEM_RESULT_UNKNOWN                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_UNKNOWN, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_INVALID_ARGUMENT|:
#define MOJO_SYSTEM_RESULT_INVALID_ARGUMENT                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_INVALID_ARGUMENT, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_DEADLINE_EXCEEDED|:
#define MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_DEADLINE_EXCEEDED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_NOT_FOUND|:
#define MOJO_SYSTEM_RESULT_NOT_FOUND                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_NOT_FOUND, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_ALREADY_EXISTS|:
#define MOJO_SYSTEM_RESULT_ALREADY_EXISTS                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_ALREADY_EXISTS, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_PERMISSION_DENIED|:
#define MOJO_SYSTEM_RESULT_PERMISSION_DENIED                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_PERMISSION_DENIED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_RESOURCE_EXHAUSTED|:
#define MOJO_SYSTEM_RESULT_RESOURCE_EXHAUSTED          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_RESOURCE_EXHAUSTED, \
                   MOJO_ERROR_SPACE_SYSTEM, MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_FAILED_PRECONDITION|:
#define MOJO_SYSTEM_RESULT_FAILED_PRECONDITION          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_FAILED_PRECONDITION, \
                   MOJO_ERROR_SPACE_SYSTEM, MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_SYSTEM_RESULT_BUSY                         \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_FAILED_PRECONDITION, \
                   MOJO_ERROR_SPACE_SYSTEM,             \
                   MOJO_SYSTEM_ERROR_CODE_FAILED_PRECONDITION_SUBCODE_BUSY)

// Error code |MOJO_ERROR_CODE_ABORTED|:
#define MOJO_SYSTEM_RESULT_ABORTED                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_ABORTED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_OUT_OF_RANGE|:
#define MOJO_SYSTEM_RESULT_OUT_OF_RANGE                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_OUT_OF_RANGE, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_UNIMPLEMENTED|:
#define MOJO_SYSTEM_RESULT_UNIMPLEMENTED                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_UNIMPLEMENTED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_INTERNAL|:
#define MOJO_SYSTEM_RESULT_INTERNAL                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_INTERNAL, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Error code |MOJO_ERROR_CODE_UNAVAILABLE|:
#define MOJO_SYSTEM_RESULT_UNAVAILABLE                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_UNAVAILABLE, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_SYSTEM_RESULT_SHOULD_WAIT                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_UNAVAILABLE, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_SYSTEM_ERROR_CODE_UNAVAILABLE_SUBCODE_SHOULD_WAIT)

// Error code |MOJO_ERROR_CODE_DATA_LOSS|:
#define MOJO_SYSTEM_RESULT_DATA_LOSS                                   \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_DATA_LOSS, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

#endif  // MOJO_PUBLIC_C_INCLUDE_MOJO_SYSTEM_RESULT_H_
