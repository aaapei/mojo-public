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
    const std::string& connection_url,
    const std::string& remote_url,
    InterfaceRequest<ServiceProvider> local_services)
    : connection_url_(connection_url),
      remote_url_(remote_url),
      local_binding_(this) {
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

const std::string& ServiceRegistry::GetConnectionURL() {
  return connection_url_;
}

const std::string& ServiceRegistry::GetRemoteApplicationURL() {
  return remote_url_;
}

void ServiceRegistry::ConnectToService(const String& service_name,
                                       ScopedMessagePipeHandle client_handle) {
  service_connector_registry_.ConnectToService(this, service_name,
                                               &client_handle);
}

}  // namespace internal
}  // namespace mojo
