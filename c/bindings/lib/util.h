// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_LIB_UTIL_H_
#define MOJO_PUBLIC_C_BINDINGS_LIB_UTIL_H_

// Rounds-up |num| to 8. The result is undefined if this results in an overflow.
#define MOJOM_INTERNAL_ROUND_TO_8(num) (((num) + 7) & ~7)

#endif  // MOJO_PUBLIC_C_BINDINGS_LIB_UTIL_H_
