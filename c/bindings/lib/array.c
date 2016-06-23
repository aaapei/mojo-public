// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/array.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "mojo/public/c/bindings/buffer.h"
#include "mojo/public/c/bindings/lib/util.h"

struct MojomArrayHeader* MojomArray_New(struct MojomBuffer* buf,
                                        uint32_t num_elements,
                                        uint32_t element_byte_size) {
  assert(buf);

  uint64_t num_bytes = sizeof(struct MojomArrayHeader) +
                       (uint64_t)num_elements * element_byte_size;
  if (num_bytes > UINT32_MAX)
    return NULL;

  struct MojomArrayHeader* arr =
      (struct MojomArrayHeader*)MojomBuffer_Allocate(buf, (uint32_t)num_bytes);
  if (arr == NULL)
    return NULL;

  assert((uintptr_t)arr + MOJOM_INTERNAL_ROUND_TO_8(num_bytes) ==
         (uintptr_t)buf->buf + buf->num_bytes_used);

  arr->num_elements = num_elements;
  arr->num_bytes = MOJOM_INTERNAL_ROUND_TO_8((uint32_t)num_bytes);

  return arr;
}
