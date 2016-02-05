// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_impl.h"

#include <utility>

#include "mojo/public/cpp/application/application_delegate.h"
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
  MessagePipe pipe;
  shell_->CreateApplicationConnector(
      MakeRequest<ApplicationConnector>(pipe.handle1.Pass()));
  return InterfaceHandle<ApplicationConnector>(pipe.handle0.Pass(), 0u);
}

ApplicationConnection* ApplicationImpl::ConnectToApplication(
    const String& application_url) {
  MOJO_CHECK(shell_);
  ServiceProviderPtr local_services;
  InterfaceRequest<ServiceProvider> local_request = GetProxy(&local_services);
  ServiceProviderPtr remote_services;
  shell_->ConnectToApplication(application_url, GetProxy(&remote_services),
                               local_services.Pass());
  internal::ServiceRegistry* registry = new internal::ServiceRegistry(
      this, application_url, application_url, remote_services.Pass(),
      local_request.Pass());
  outgoing_service_registries_.emplace_back(registry);
  return registry;
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

void ApplicationImpl::Initialize(ShellPtr shell,
                                 Array<String> args,
                                 const mojo::String& url) {
  shell_ = shell.Pass();
  shell_.set_connection_error_handler([this]() {
    delegate_->Quit();
    incoming_service_registries_.clear();
    outgoing_service_registries_.clear();
    Terminate();
  });
  url_ = url;
  args_ = args.To<std::vector<std::string>>();
  delegate_->Initialize(this);
}

void ApplicationImpl::AcceptConnection(
    const String& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    ServiceProviderPtr exposed_services,
    const String& url) {
  std::unique_ptr<internal::ServiceRegistry> registry(
      new internal::ServiceRegistry(this, url, requestor_url,
                                    exposed_services.Pass(), services.Pass()));
  if (!delegate_->ConfigureIncomingConnection(registry.get()))
    return;
  incoming_service_registries_.push_back(std::move(registry));
}

void ApplicationImpl::RequestQuit() {
  delegate_->Quit();
  Terminate();
}

}  // namespace mojo
