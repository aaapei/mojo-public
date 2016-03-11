// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_H_
#define MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"

typedef int MojoPlatformHandle;  // Unix file descriptor

#ifdef __cplusplus
extern "C" {
#endif

// Wraps |platform_handle| in a MojoHandle so that it can transported. Returns
// MOJO_RESULT_OK on success, all other results indicate failure. This takes
// ownership of |platform_handle|, regardless of whether this succeeds.
MojoResult MojoCreatePlatformHandleWrapper(MojoPlatformHandle platform_handle,
                                           MojoHandle* wrapper);

// Extracts |platform_handle| from |wrapper|. Returns MOJO_RESULT_OK on success,
// all other results indicate failure. If this succeeds, it causes |wrapper| to
// relinquish ownership of |platform_handle|, so MojoClose'ing |wrapper| will no
// longer close the underlying |platform_handle|. Never the less, it is still
// neccessary to MojoClose |wrapper|, but this will not affect the underlying
// descriptor after this call.
MojoResult MojoExtractPlatformHandle(MojoHandle wrapper,
                                     MojoPlatformHandle* platform_handle);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_H_
