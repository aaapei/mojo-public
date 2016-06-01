// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_runner.h"

#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace mojo {
namespace {
bool g_running = false;
}  // namespace

ApplicationRunner::ApplicationRunner(
    std::unique_ptr<ApplicationDelegate> delegate)
    : delegate_(std::move(delegate)) {}

ApplicationRunner::~ApplicationRunner() {
  assert(!delegate_);
}

MojoResult ApplicationRunner::Run(MojoHandle app_request_handle) {
  MOJO_DCHECK(!g_running)
      << "Another ApplicationRunner::Run() is already running!";

  g_running = true;
  {
    RunLoop loop;
    ApplicationImpl app(delegate_.get(),
                        InterfaceRequest<Application>(MakeScopedHandle(
                            MessagePipeHandle(app_request_handle))));
    loop.Run();
  }

  delegate_.reset();

  g_running = false;

  return MOJO_RESULT_OK;
}

}  // namespace mojo
