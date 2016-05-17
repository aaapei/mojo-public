// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_H_
#define MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_H_

#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {

class ApplicationDelegate;
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
class ApplicationImpl : public ApplicationImplBase {
 public:
  // Does not take ownership of |delegate|, which must remain valid for the
  // lifetime of ApplicationImpl.
  ApplicationImpl(ApplicationDelegate* delegate,
                  InterfaceRequest<Application> request);
  ~ApplicationImpl() override;

 private:
  // |ApplicationImplBase| implementation/overrides:
  void OnInitialize() final;
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) final;
  void OnQuit() final;

  ApplicationDelegate* const delegate_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ApplicationImpl);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_H_
