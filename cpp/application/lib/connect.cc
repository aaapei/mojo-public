// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/connect.h"

#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {

InterfaceHandle<ApplicationConnector> CreateApplicationConnector(Shell* shell) {
  InterfaceHandle<ApplicationConnector> application_connector;
  shell->CreateApplicationConnector(GetProxy(&application_connector));
  return application_connector;
}

}  // namespace mojo
