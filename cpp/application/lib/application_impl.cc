// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_impl.h"

#include <utility>

#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/connection_context.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/environment/logging.h"

namespace mojo {

ApplicationImpl::ApplicationImpl(ApplicationDelegate* delegate,
                                 InterfaceRequest<Application> request)
    : delegate_(delegate), application_binding_(this, request.Pass()) {}

ApplicationImpl::~ApplicationImpl() {}

bool ApplicationImpl::HasArg(const std::string& arg) const {
  return std::find(args_.begin(), args_.end(), arg) != args_.end();
}

void ApplicationImpl::WaitForInitialize() {
  if (!shell_)
    application_binding_.WaitForIncomingMethodCall();
}

void ApplicationImpl::UnbindConnections(
    InterfaceRequest<Application>* application_request,
    ShellPtr* shell) {
  *application_request = application_binding_.Unbind();
  shell->Bind(shell_.PassInterfaceHandle());
}

void ApplicationImpl::Initialize(InterfaceHandle<Shell> shell,
                                 Array<String> args,
                                 const mojo::String& url) {
  shell_ = ShellPtr::Create(std::move(shell));
  shell_.set_connection_error_handler([this]() {
    delegate_->Quit();
    service_provider_impls_.clear();
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
  std::unique_ptr<ServiceProviderImpl> service_provider_impl(
      new ServiceProviderImpl(
          ConnectionContext(ConnectionContext::Type::INCOMING, requestor_url,
                            url),
          services.Pass()));
  if (!delegate_->ConfigureIncomingConnection(service_provider_impl.get()))
    return;
  service_provider_impls_.push_back(std::move(service_provider_impl));
}

void ApplicationImpl::RequestQuit() {
  delegate_->Quit();
  Terminate();
}

}  // namespace mojo
