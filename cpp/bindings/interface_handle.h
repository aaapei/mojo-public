// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_
#define MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_

#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {

// InterfaceHandle stores necessary information to communicate with a remote
// interface implementation, which could be used to construct an InterfacePtr.
template <typename Interface>
class InterfaceHandle {
 public:
  InterfaceHandle() : version_(0u) {}

  InterfaceHandle(ScopedMessagePipeHandle handle, uint32_t version)
      : handle_(handle.Pass()), version_(version) {}

  InterfaceHandle(InterfaceHandle&& other)
      : handle_(other.handle_.Pass()), version_(other.version_) {
    other.version_ = 0u;
  }

  ~InterfaceHandle() {}

  InterfaceHandle& operator=(InterfaceHandle&& other) {
    if (this != &other) {
      handle_ = other.handle_.Pass();
      version_ = other.version_;
      other.version_ = 0u;
    }

    return *this;
  }

  bool is_valid() const { return handle_.is_valid(); }

  ScopedMessagePipeHandle PassHandle() { return handle_.Pass(); }
  const ScopedMessagePipeHandle& handle() const { return handle_; }
  void set_handle(ScopedMessagePipeHandle handle) { handle_ = handle.Pass(); }

  uint32_t version() const { return version_; }
  void set_version(uint32_t version) { version_ = version; }

 private:
  ScopedMessagePipeHandle handle_;
  uint32_t version_;

  MOJO_MOVE_ONLY_TYPE(InterfaceHandle);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_
