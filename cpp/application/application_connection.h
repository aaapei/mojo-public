// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_APPLICATION_APPLICATION_CONNECTION_H_
#define MOJO_PUBLIC_APPLICATION_APPLICATION_CONNECTION_H_

#include <memory>
#include <string>

#include "mojo/public/cpp/application/lib/interface_factory_connector.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"

namespace mojo {

struct ConnectionContext;
class ServiceConnector;

// Represents a connection to another application. An instance of this class is
// passed to ApplicationDelegate's ConfigureIncomingConnection() method each
// time a connection is made to this app, and is returned by the
// ApplicationDelegate's ConnectToApplication() method when this app
// connects to another.
//
// To use, define a class that implements your specific service API (e.g.,
// FooImpl to implement a service named Foo). Then implement an
// InterfaceFactory<Foo> that binds instances of FooImpl to
// InterfaceRequest<Foo>s and register that on the connection like this:
//
//   connection->AddService(&factory);
//
// Or, if you have multiple factories implemented by the same type, explicitly
// specify the interface to register the factory for:
//
//   connection->AddService<Foo>(&my_foo_and_bar_factory_);
//   connection->AddService<Bar>(&my_foo_and_bar_factory_);
//
// The InterfaceFactory must outlive the ApplicationConnection.
//
// TODO(vtl): Don't get too attached to this class. I'm going to remove it.
class ApplicationConnection {
 public:
  virtual ~ApplicationConnection();

  // Makes Interface available as a service to the remote application.
  // |factory| will create implementations of Interface on demand.
  template <typename Interface>
  void AddService(InterfaceFactory<Interface>* factory) {
    GetServiceProviderImpl().AddServiceForName(
        std::unique_ptr<ServiceConnector>(
            new internal::InterfaceFactoryConnector<Interface>(factory)),
        Interface::Name_);
  }

  virtual ServiceProviderImpl& GetServiceProviderImpl() = 0;
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_APPLICATION_APPLICATION_CONNECTION_H_
