// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/lib/type_descriptor.h"

#include <stddef.h>

const struct MojomTypeDescriptorArray g_mojom_string_type_description = {
    MOJOM_TYPE_DESCRIPTOR_TYPE_POD,  // elem_descriptor
    NULL,                            // elem_table
    0,                               // num_elements
    false,                           // nullable
};
