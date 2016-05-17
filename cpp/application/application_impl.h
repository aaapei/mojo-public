// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_H_
#define MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {

class ServiceProviderImpl;

// Implements the Application interface, which the shell uses for basic
// communication with an application (e.g., to connect clients to services
// provided by an application). Also provides the application access to the
// Shell, which, e.g., may be used by an application to connect to other
// services.
//
// Typically, you create one or more classes implementing your APIs (e.g.,
// FooImpl implementing Foo). See bindings/binding.h for more information. Then
// you implement an mojo::ApplicationDelegate whose
// ConfigureIncomingConnection() adds services to each connection. Finally, you
// instantiate your delegate and pass it to an ApplicationRunner, which will
// create the ApplicationImpl and then run a message (or run) loop.
class ApplicationImpl : public Application {
 public:
  // Does not take ownership of |delegate|, which must remain valid for the
  // lifetime of ApplicationImpl.
  ApplicationImpl(ApplicationDelegate* delegate,
                  InterfaceRequest<Application> request);
  ~ApplicationImpl() override;

  // Quits the main run loop for this application.
  // TODO(vtl): This is implemented in application_runner.cc (for example). Its
  // presence here is pretty dubious.
  static void Terminate();

  // The Mojo shell. This will return a valid pointer after Initialize() has
  // been invoked. It will remain valid until UnbindConnections() is invoked or
  // the ApplicationImpl is destroyed.
  Shell* shell() const { return shell_.get(); }

  const std::string& url() const { return url_; }

  // Returns any initial configuration arguments, passed by the Shell.
  const std::vector<std::string>& args() const { return args_; }
  bool HasArg(const std::string& arg) const;

  // Blocks until the |Application| is initialized (i.e., |Initialize()| is
  // received), if it is not already.
  void WaitForInitialize();

  // Unbinds the Shell and Application connections. Can be used to re-bind the
  // handles to another implementation of ApplicationImpl, for instance when
  // running apptests.
  void UnbindConnections(InterfaceRequest<Application>* application_request,
                         ShellPtr* shell);

  // |Application| implementation.
  void Initialize(InterfaceHandle<Shell> shell,
                  Array<String> args,
                  const mojo::String& url) override;
  void AcceptConnection(const String& requestor_url,
                        InterfaceRequest<ServiceProvider> services,
                        InterfaceHandle<ServiceProvider> exposed_services,
                        const String& url) override;
  void RequestQuit() override;

 private:
  std::vector<std::unique_ptr<ServiceProviderImpl>> service_provider_impls_;
  ApplicationDelegate* delegate_;
  Binding<Application> application_binding_;
  ShellPtr shell_;
  std::string url_;
  std::vector<std::string> args_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ApplicationImpl);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_H_
