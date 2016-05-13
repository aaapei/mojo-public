// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/lib/service_registry.h"

#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/service_connector.h"

namespace mojo {
namespace internal {

ServiceRegistry::ServiceRegistry() {}

ServiceRegistry::ServiceRegistry(
    const ConnectionContext& connection_context,
    InterfaceRequest<ServiceProvider> local_services)
    : service_provider_impl_(connection_context, local_services.Pass()) {}

ServiceRegistry::~ServiceRegistry() {}

void ServiceRegistry::SetServiceConnectorForName(
    ServiceConnector* service_connector,
    const std::string& interface_name) {
  service_provider_impl_.AddServiceForName(
      std::unique_ptr<ServiceConnector>(service_connector), interface_name);
}

void ServiceRegistry::RemoveServiceConnectorForName(
    const std::string& interface_name) {
  service_provider_impl_.RemoveServiceForName(interface_name);
}

ServiceProviderImpl& ServiceRegistry::GetServiceProviderImpl() {
  return service_provider_impl_;
}

const ConnectionContext& ServiceRegistry::GetConnectionContext() const {
  return service_provider_impl_.connection_context();
}

const std::string& ServiceRegistry::GetConnectionURL() {
  return service_provider_impl_.connection_context().connection_url;
}

const std::string& ServiceRegistry::GetRemoteApplicationURL() {
  return service_provider_impl_.connection_context().remote_url;
}

}  // namespace internal
}  // namespace mojo
