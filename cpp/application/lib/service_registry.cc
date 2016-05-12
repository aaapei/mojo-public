// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/lib/service_registry.h"

#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/service_connector.h"

namespace mojo {
namespace internal {

ServiceRegistry::ServiceRegistry() : local_binding_(this) {}

ServiceRegistry::ServiceRegistry(
    const ConnectionContext& connection_context,
    InterfaceRequest<ServiceProvider> local_services)
    : connection_context_(connection_context), local_binding_(this) {
  if (local_services.is_pending())
    local_binding_.Bind(local_services.Pass());
}

ServiceRegistry::~ServiceRegistry() {}

void ServiceRegistry::SetServiceConnectorForName(
    ServiceConnector* service_connector,
    const std::string& interface_name) {
  service_connector_registry_.SetServiceConnectorForName(
      std::unique_ptr<ServiceConnector>(service_connector), interface_name);
}

void ServiceRegistry::RemoveServiceConnectorForName(
    const std::string& interface_name) {
  service_connector_registry_.RemoveServiceConnectorForName(interface_name);
}

const ConnectionContext& ServiceRegistry::GetConnectionContext() const {
  return connection_context_;
}

const std::string& ServiceRegistry::GetConnectionURL() {
  return connection_context_.connection_url;
}

const std::string& ServiceRegistry::GetRemoteApplicationURL() {
  return connection_context_.remote_url;
}

void ServiceRegistry::ConnectToService(const String& service_name,
                                       ScopedMessagePipeHandle client_handle) {
  service_connector_registry_.ConnectToService(connection_context_,
                                               service_name, &client_handle);
}

}  // namespace internal
}  // namespace mojo
