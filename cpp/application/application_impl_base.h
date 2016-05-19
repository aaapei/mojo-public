// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_BASE_H_
#define MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {

class ServiceProviderImpl;

// Base helper class for implementing the |Application| interface, which the
// shell uses for basic communication with an application (e.g., to connect
// clients to services provided by an application).
//
// To use this class, subclass it and implement/override the required methods
// (see below).
//
// TODO(vtl): ApplicationRunners should take this instead of an
// ApplicationDelegate. Write more here when that's true (it's pretty hard to
// use this class in the current setup).
class ApplicationImplBase : public Application {
 public:
  ApplicationImplBase();
  ~ApplicationImplBase() override;

  // Binds the given |Application| request to this object. This must be done
  // with the message (run) loop available/running, and this will cause this
  // object to start serving requests (via that message loop).
  void Bind(InterfaceRequest<Application> application_request);

  // Quits the main run loop for this application.
  // TODO(vtl): This is implemented in application_runner.cc (for example). Its
  // presence here is pretty dubious.
  static void Terminate();

  // This will be valid after |Initialize()| has been received and remain valid
  // until this object is destroyed.
  Shell* shell() const { return shell_.get(); }

  // Returns any initial configuration arguments, passed by the shell.
  const std::vector<std::string>& args() const { return args_; }
  bool HasArg(const std::string& arg) const;

  const std::string& url() const { return url_; }

  // Methods to be implemented/overridden by subclasses:

  // Called after |Initialize()| has been received (|shell()|, |args()|, and
  // |url()| will be valid when this is called. The default implementation does
  // nothing.
  virtual void OnInitialize() {}

  // Called when another application connects to this application (i.e., we
  // receive |AcceptConnection()|). This should either configure what services
  // are "provided" (made available via a |ServiceProvider|) to that application
  // and return true, or this may return false to reject the connection
  // entirely.
  virtual bool OnAcceptConnection(
      ServiceProviderImpl* service_provider_impl) = 0;

  // Called before quitting the main message (run) loop, i.e., before
  // |Terminate()|. The default implementation does nothing.
  virtual void OnQuit() {}

 private:
  // |Application| implementation. In general, you probably shouldn't call these
  // directly (but I can't really stop you).
  void Initialize(InterfaceHandle<Shell> shell,
                  Array<String> args,
                  const mojo::String& url) final;
  void AcceptConnection(const String& requestor_url,
                        InterfaceRequest<ServiceProvider> services,
                        InterfaceHandle<ServiceProvider> exposed_services,
                        const String& url) final;
  void RequestQuit() final;

  Binding<Application> application_binding_;

  // Set by |Initialize()|.
  ShellPtr shell_;
  std::vector<std::string> args_;
  std::string url_;

  std::vector<std::unique_ptr<ServiceProviderImpl>> service_provider_impls_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ApplicationImplBase);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_BASE_H_
