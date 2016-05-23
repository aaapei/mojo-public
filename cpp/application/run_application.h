// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_RUN_APPLICATION_H_
#define MOJO_PUBLIC_CPP_APPLICATION_RUN_APPLICATION_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"

namespace mojo {

class ApplicationImplBase;

// Sets up a message (run) loop and runs the provided application
// implementation.
//
// Typical use (where |MyApplicationImpl| is an implementation of
// |ApplicationImplBase|):
//
//   MojoResult MojoMain(MojoHandle application_request_handle) {
//     MyApplicationImpl my_app;
//     mojo::RunApplication(application_request_handle, &my_app);
//     return MOJO_RESULT_OK;
//   }
//
// Note that this may actually be used on any thread (that is not already
// running a message loop of some sort).
//
// |*application_impl| must remain alive until the message loop starts running
// (thus it may own itself).
//
// TODO(vtl): Possibly this should return a |MojoResult| and
// |TerminateApplication()| should take a |MojoResult| argument (which this
// returns), but to communicate that value requires TLS.
// TODO(vtl): Maybe this should be like |Environment|, with different possible
// implementations (e.g., "standalone" vs "chromium"). However,
// |ApplicationRunnerChromium| currently has additional goop specific to the
// main thread. Probably we should have additional "MainRunApplication()" and
// "MainTerminateApplication()" functions.
void RunApplication(MojoHandle application_request_handle,
                    ApplicationImplBase* application_impl);

// Terminates the currently-running application. This may only be called from
// "inside" |RunApplication()| (i.e., it is on the stack, which means that the
// message loop is running). This will cause |RunApplication()| to return "soon"
// (assuming the message loop is not blocked processing some task). This may
// cause queued work to *not* be executed.
void TerminateApplication();

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_RUN_APPLICATION_H_
