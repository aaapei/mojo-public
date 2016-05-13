// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_impl.h"

#include <utility>

#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/connection_context.h"
#include "mojo/public/cpp/application/lib/service_registry.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {

ApplicationImpl::ApplicationImpl(ApplicationDelegate* delegate,
                                 InterfaceRequest<Application> request)
    : delegate_(delegate), binding_(this, request.Pass()) {}

ApplicationImpl::~ApplicationImpl() {}

bool ApplicationImpl::HasArg(const std::string& arg) const {
  return std::find(args_.begin(), args_.end(), arg) != args_.end();
}

InterfaceHandle<ApplicationConnector>
ApplicationImpl::CreateApplicationConnector() {
  MOJO_CHECK(shell_);
  InterfaceHandle<ApplicationConnector> application_connector;
  shell_->CreateApplicationConnector(GetProxy(&application_connector));
  return application_connector;
}

void ApplicationImpl::WaitForInitialize() {
  if (!shell_)
    binding_.WaitForIncomingMethodCall();
}

void ApplicationImpl::UnbindConnections(
    InterfaceRequest<Application>* application_request,
    ShellPtr* shell) {
  *application_request = binding_.Unbind();
  shell->Bind(shell_.PassInterfaceHandle());
}

void ApplicationImpl::Initialize(InterfaceHandle<Shell> shell,
                                 Array<String> args,
                                 const mojo::String& url) {
  shell_ = ShellPtr::Create(std::move(shell));
  shell_.set_connection_error_handler([this]() {
    delegate_->Quit();
    incoming_service_registries_.clear();
    Terminate();
  });
  url_ = url;
  args_ = args.To<std::vector<std::string>>();
  delegate_->Initialize(this);
}

void ApplicationImpl::AcceptConnection(
    const String& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    InterfaceHandle<ServiceProvider> exposed_services,
    const String& url) {
  // Note: The shell no longer actually connects |exposed_services|, so a) we
  // never actually get valid |exposed_services| here, b) it should be OK to
  // drop it on the floor.
  MOJO_LOG_IF(ERROR, exposed_services)
      << "DEPRECATED: exposed_services is going away";
  std::unique_ptr<internal::ServiceRegistry> registry(
      new internal::ServiceRegistry(
          ConnectionContext(ConnectionContext::Type::INCOMING, requestor_url,
                            url),
          services.Pass()));
  if (!delegate_->ConfigureIncomingConnection(
          &registry->GetServiceProviderImpl()))
    return;
  incoming_service_registries_.push_back(std::move(registry));
}

void ApplicationImpl::RequestQuit() {
  delegate_->Quit();
  Terminate();
}

}  // namespace mojo
