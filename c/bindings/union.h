// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_UNION_H_
#define MOJO_PUBLIC_C_BINDINGS_UNION_H_

#include <stddef.h>
#include <stdint.h>

#include "mojo/public/c/bindings/buffer.h"
#include "mojo/public/c/bindings/lib/type_descriptor.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

struct MojomUnionLayout {
  // |num_bytes|, the number of bytes used in this union, includes the size of
  // the |num_bytes| and |tag| fields, along with data.
  uint32_t size;
  uint32_t tag;
  union {
    // To be used when the data is a pointer.
    union MojomPointer pointer;

    // This is here to force this union data to be at least 8 bytes.
    uint64_t force_size_;
  } data;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomUnionLayout) == 16,
                   "struct MojomUnionLayout must be 16 bytes.");

// Returns the number of bytes required to serialize |in_union_data|'s active
// field, not including the union layout (as described by |struct
// MojomUnionLayout|) itself.
// |in_type_desc| is the generated type that describes the tags of the pointer
// and handle types in the given union.
size_t MojomUnion_ComputeSerializedSize(
    const struct MojomTypeDescriptorUnion* in_type_desc,
    const struct MojomUnionLayout* in_union_data);

// Encodes the mojom union described by the |inout_union| buffer; note that any
// references from the union are also in the buffer backed by |inout_union|, and
// they are recursively encoded. Encodes all pointers to relative offsets, and
// encodes all handles by moving them into |inout_handles_buffer| and encoding
// the index into the handle.
// |in_type_desc|: Describes the pointer and handle fields of the mojom union.
// |inout_union|: Contains the union, and any other references outside the
//                union.
// |in_union_size|:  Size of the buffer backed by |inout_union| in bytes.
// |inout_handles_buffer|:
//   A buffer used to record handles during encoding. The |num_handles_used|
//   field can be used to determine how many handles were moved into this
//   buffer.
void MojomUnion_EncodePointersAndHandles(
    const struct MojomTypeDescriptorUnion* in_type_desc,
    struct MojomUnionLayout* inout_union,
    uint32_t in_union_size,
    struct MojomHandleBuffer* inout_handles_buffer);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_UNION_H_
