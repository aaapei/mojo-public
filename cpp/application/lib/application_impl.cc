// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_impl.h"

#include "mojo/public/cpp/application/application_delegate.h"

namespace mojo {

ApplicationImpl::ApplicationImpl(ApplicationDelegate* delegate,
                                 InterfaceRequest<Application> request)
    : ApplicationImplBase(request.Pass()), delegate_(delegate) {}

ApplicationImpl::~ApplicationImpl() {}

void ApplicationImpl::OnInitialize() {
  delegate_->Initialize(this);
}

bool ApplicationImpl::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  return delegate_->ConfigureIncomingConnection(service_provider_impl);
}

void ApplicationImpl::OnQuit() {
  delegate_->Quit();
}

}  // namespace mojo
