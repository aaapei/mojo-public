// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_ARRAY_H_
#define MOJO_PUBLIC_C_BINDINGS_ARRAY_H_

#include <stdint.h>

#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

// The fields below are just the header of a mojom array. The bytes that
// immediately follow this struct consist of |num_bytes -
// sizeof(MojomArrayHeader)| bytes describing |num_elements| elements of the
// array.
struct MojomArrayHeader {
  // num_bytes includes the size of this struct along with the
  // accompanying array bytes that follow these fields.
  uint32_t num_bytes;
  uint32_t num_elements;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomArrayHeader) == 8,
                   "struct MojomArrayHeader must be 8 bytes.");

// This union is used to represent references to a mojom array.
union MojomArrayHeaderPtr {
  // |ptr| is used to access the array when it hasn't been encoded yet.
  struct MojomArrayHeader* ptr;
  // |offset| is used to access the array after it has been encoded.
  uint64_t offset;
};
MOJO_STATIC_ASSERT(sizeof(union MojomArrayHeaderPtr) == 8,
                   "union MojomArrayHeaderPtr must be 8 bytes.");

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_ARRAY_H_
