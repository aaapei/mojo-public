// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_RUN_APPLICATION_H_
#define MOJO_PUBLIC_CPP_APPLICATION_RUN_APPLICATION_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"

namespace mojo {

class ApplicationImplBase;

// |RunMainApplication()| and |RunApplication()| set up a message (run) loop and
// run the provided application implementation. |RunMainApplication()| should be
// used on the application's main thread (e.g., from |MojoMain()|) and may do
// additional initialization. (In the case of the "standalone" implementation,
// the two are equivalent.) The return value will be the one provided to
// |TerminateMainApplication()| or |TerminateApplication()|, respectively.
//
// Typical use (where |MyApplicationImpl| is an implementation of
// |ApplicationImplBase|):
//
//   MojoResult MojoMain(MojoHandle application_request_handle) {
//     MyApplicationImpl my_app;
//     return mojo::RunMainApplication(application_request_handle, &my_app);
//   }
//
// |RunApplication()| may be used on secondary threads (that are not already
// running a message loop of some sort).
//
// |*application_impl| must remain alive until the message loop starts running
// (thus it may own itself).
MojoResult RunMainApplication(MojoHandle application_request_handle,
                              ApplicationImplBase* application_impl);
MojoResult RunApplication(MojoHandle application_request_handle,
                          ApplicationImplBase* application_impl);

// |TerminateMainApplication()| and |TerminateApplication()| terminate the
// application that is running on the current thread. They may only be called
// from "inside" |RunMainApplication()| and |RunApplication()| respectively
// (i.e., |Run[Main]Application()| is on the stack, which means that the message
// loop is running). They will cause |Run[Main]Application()| to return "soon"
// (assuming the message loop is not blocked processing some task), with return
// value |result|. They may cause queued work to *not* be executed. They should
// be executed at most once (per |Run[Main]Application()|).
void TerminateMainApplication(MojoResult result);
void TerminateApplication(MojoResult result);

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_RUN_APPLICATION_H_
