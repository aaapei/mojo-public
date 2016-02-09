// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_SYSTEM_MAIN_H_
#define MOJO_PUBLIC_C_SYSTEM_MAIN_H_

#include "mojo/public/c/system/types.h"

// Implement MojoMain directly as the entry point for an application.
//
// MojoResult MojoMain(MojoHandle application_request) {
//   ...
// }

#if defined(__cplusplus)
extern "C" {
#endif

__attribute__((visibility("default"))) MojoResult MojoMain(
    MojoHandle application_request);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MOJO_PUBLIC_C_SYSTEM_MAIN_H_
