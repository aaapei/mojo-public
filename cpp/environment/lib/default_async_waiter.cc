// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/environment/lib/default_async_waiter.h"

#include <assert.h>
#include <mojo/environment/async_waiter.h>

#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/public/cpp/utility/run_loop_handler.h"

namespace mojo {

namespace {

// RunLoopHandler implementation used for a request to AsyncWait(). There are
// two ways RunLoopHandlerImpl is deleted:
// . when the handle is ready (or errored).
// . when CancelWait() is invoked.
class RunLoopHandlerImpl : public RunLoopHandler {
 public:
  RunLoopHandlerImpl(MojoAsyncWaitCallback callback, void* closure)
      : id_(0u), callback_(callback), closure_(closure) {}

  ~RunLoopHandlerImpl() override { RunLoop::current()->RemoveHandler(id_); }

  void set_id(Id id) { id_ = id; }

  // RunLoopHandler:
  void OnHandleReady(Id /*id*/) override { NotifyCallback(MOJO_RESULT_OK); }

  void OnHandleError(Id /*id*/, MojoResult result) override {
    NotifyCallback(result);
  }

 private:
  void NotifyCallback(MojoResult result) {
    // Delete this to unregister the handle. That way if the callback
    // reregisters everything is ok.
    MojoAsyncWaitCallback callback = callback_;
    void* closure = closure_;
    delete this;

    callback(closure, result);
  }

  Id id_;
  const MojoAsyncWaitCallback callback_;
  void* const closure_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(RunLoopHandlerImpl);
};

MojoAsyncWaitID AsyncWait(MojoHandle handle,
                          MojoHandleSignals signals,
                          MojoDeadline deadline,
                          MojoAsyncWaitCallback callback,
                          void* closure) {
  RunLoop* run_loop = RunLoop::current();
  assert(run_loop);

  // |run_loop_handler| is destroyed either when the handle is ready or if
  // CancelWait is invoked.
  RunLoopHandlerImpl* run_loop_handler =
      new RunLoopHandlerImpl(callback, closure);
  run_loop_handler->set_id(run_loop->AddHandler(
      run_loop_handler, Handle(handle), signals, deadline));
  return reinterpret_cast<MojoAsyncWaitID>(run_loop_handler);
}

void CancelWait(MojoAsyncWaitID wait_id) {
  delete reinterpret_cast<RunLoopHandlerImpl*>(wait_id);
}

}  // namespace

namespace internal {

const MojoAsyncWaiter kDefaultAsyncWaiter = {AsyncWait, CancelWait};

}  // namespace internal

}  // namespace mojo
