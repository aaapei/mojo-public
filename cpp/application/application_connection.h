// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_APPLICATION_APPLICATION_CONNECTION_H_
#define MOJO_PUBLIC_APPLICATION_APPLICATION_CONNECTION_H_

#include "mojo/public/cpp/application/service_provider_impl.h"

namespace mojo {

// Represents a connection to another application. An instance of this class is
// passed to ApplicationDelegate's ConfigureIncomingConnection() method each
// time a connection is made to this app, and is returned by the
// ApplicationDelegate's ConnectToApplication() method when this app
// connects to another.
// TODO(vtl): Don't get too attached to this class. I'm going to remove it.
class ApplicationConnection {
 public:
  virtual ~ApplicationConnection();

  virtual ServiceProviderImpl& GetServiceProviderImpl() = 0;
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_APPLICATION_APPLICATION_CONNECTION_H_
