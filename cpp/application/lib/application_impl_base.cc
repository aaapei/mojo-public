// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_impl_base.h"

#include <algorithm>
#include <utility>

#include "mojo/public/cpp/application/connection_context.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/environment/logging.h"

namespace mojo {

ApplicationImplBase::ApplicationImplBase() : application_binding_(this) {}

ApplicationImplBase::~ApplicationImplBase() {}

void ApplicationImplBase::Bind(
    InterfaceRequest<Application> application_request) {
  application_binding_.Bind(application_request.Pass());
}

bool ApplicationImplBase::HasArg(const std::string& arg) const {
  return std::find(args_.begin(), args_.end(), arg) != args_.end();
}

void ApplicationImplBase::Initialize(InterfaceHandle<Shell> shell,
                                     Array<String> args,
                                     const mojo::String& url) {
  shell_ = ShellPtr::Create(std::move(shell));
  shell_.set_connection_error_handler([this]() {
    OnQuit();
    service_provider_impls_.clear();
    Terminate();
  });
  url_ = url;
  args_ = args.To<std::vector<std::string>>();
  OnInitialize();
}

void ApplicationImplBase::AcceptConnection(
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
  if (!OnAcceptConnection(service_provider_impl.get()))
    return;
  service_provider_impls_.push_back(std::move(service_provider_impl));
}

void ApplicationImplBase::RequestQuit() {
  OnQuit();
  Terminate();
}

}  // namespace mojo
