// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/run_application.h"

#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/public/interfaces/application/application.mojom.h"

namespace mojo {

void RunApplication(MojoHandle application_request_handle,
                    ApplicationImplBase* application_impl) {
  // TODO(vtl): Possibly we should have an assertion that we're not running, but
  // that requires TLS.

  RunLoop loop;
  application_impl->Bind(InterfaceRequest<Application>(
      MakeScopedHandle(MessagePipeHandle(application_request_handle))));
  loop.Run();

  // TODO(vtl): Should we unbind stuff here? (Should there be "will start"/"did
  // stop" notifications to the |ApplicationImplBase|?)
}

void TerminateApplication() {
  RunLoop::current()->Quit();
}

}  // namespace mojo
