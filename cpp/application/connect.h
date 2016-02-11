// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_CONNECT_H_
#define MOJO_PUBLIC_CPP_APPLICATION_CONNECT_H_

#include "mojo/public/interfaces/application/application_connector.mojom.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {

// TODO(vardhan): Replaces uses of ConnectToService(.., InterfacePtr<>) with the
// ConnectToService(.., InterfaceHandle<>) alternative instead, and get
// rid of following 3 functions.

// Binds |ptr| to a remote implementation of Interface from |service_provider|.
template <typename Interface>
inline void ConnectToService(ServiceProvider* service_provider,
                             InterfacePtr<Interface>* ptr) {
  MessagePipe pipe;
  ptr->Bind(InterfaceHandle<Interface>(pipe.handle0.Pass(), 0u));
  service_provider->ConnectToService(Interface::Name_, pipe.handle1.Pass());
}

// Binds |ptr| to a remote implementation of Interface from |application_url|.
template <typename Interface>
inline void ConnectToService(Shell* shell,
                             const std::string& application_url,
                             InterfacePtr<Interface>* ptr) {
  ServiceProviderPtr service_provider;
  shell->ConnectToApplication(application_url, GetProxy(&service_provider),
                              nullptr);
  ConnectToService(service_provider.get(), ptr);
}

// Binds |ptr| to a remote implementation of Interface from |application_url|.
template <typename Interface>
inline void ConnectToService(ApplicationConnector* application_connector,
                             const std::string& application_url,
                             InterfacePtr<Interface>* ptr) {
  ServiceProviderPtr service_provider;
  application_connector->ConnectToApplication(
      application_url, GetProxy(&service_provider), nullptr);
  ConnectToService(service_provider.get(), ptr);
}

// Binds |ptr| to a remote implementation of Interface from |service_provider|.
template <typename Interface>
inline void ConnectToService(ServiceProvider* service_provider,
                             InterfaceHandle<Interface>* ptr) {
  MessagePipe pipe;
  *ptr = InterfaceHandle<Interface>(pipe.handle0.Pass(), 0u);
  service_provider->ConnectToService(Interface::Name_, pipe.handle1.Pass());
}

// Binds |ptr| to a remote implementation of Interface from |application_url|.
template <typename Interface>
inline void ConnectToService(Shell* shell,
                             const std::string& application_url,
                             InterfaceHandle<Interface>* ptr) {
  ServiceProviderPtr service_provider;
  shell->ConnectToApplication(application_url, GetProxy(&service_provider),
                              nullptr);
  ConnectToService(service_provider.get(), ptr);
}

// Binds |ptr| to a remote implementation of Interface from |application_url|.
template <typename Interface>
inline void ConnectToService(ApplicationConnector* application_connector,
                             const std::string& application_url,
                             InterfaceHandle<Interface>* ptr) {
  ServiceProviderPtr service_provider;
  application_connector->ConnectToApplication(
      application_url, GetProxy(&service_provider), nullptr);
  ConnectToService(service_provider.get(), ptr);
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_CONNECT_H_
