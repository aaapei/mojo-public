// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_REQUEST_H_
#define MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_REQUEST_H_

#include <cstddef>

#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {

template <typename I>
class InterfacePtr;

template <typename I>
class InterfaceHandle;

// Represents a request from a remote client for an implementation of Interface
// over a specified message pipe. The implementor of the interface should
// remove the message pipe by calling PassMessagePipe() and bind it to the
// implementation. If this is not done, the InterfaceRequest will automatically
// close the pipe on destruction. Can also represent the absence of a request
// if the client did not provide a message pipe.
template <typename Interface>
class InterfaceRequest {
 public:
  // Constructs an empty InterfaceRequest, representing that the client is not
  // requesting an implementation of Interface.
  InterfaceRequest() {}
  InterfaceRequest(std::nullptr_t) {}

  // Takes the message pipe from another InterfaceRequest.
  InterfaceRequest(InterfaceRequest&& other) { handle_ = other.handle_.Pass(); }
  InterfaceRequest& operator=(InterfaceRequest&& other) {
    handle_ = other.handle_.Pass();
    return *this;
  }

  // Assigning to nullptr resets the InterfaceRequest to an empty state,
  // closing the message pipe currently bound to it (if any).
  InterfaceRequest& operator=(std::nullptr_t) {
    handle_.reset();
    return *this;
  }

  // Binds the request to a message pipe over which Interface is to be
  // requested.  If the request is already bound to a message pipe, the current
  // message pipe will be closed.
  void Bind(ScopedMessagePipeHandle handle) { handle_ = handle.Pass(); }

  // Indicates whether the request currently contains a valid message pipe.
  bool is_pending() const { return handle_.is_valid(); }

  // Removes the message pipe from the request and returns it.
  ScopedMessagePipeHandle PassMessagePipe() { return handle_.Pass(); }

 private:
  ScopedMessagePipeHandle handle_;

  MOJO_MOVE_ONLY_TYPE(InterfaceRequest);
};

// Makes an InterfaceRequest bound to the specified message pipe. If |handle|
// is empty or invalid, the resulting InterfaceRequest will represent the
// absence of a request.
template <typename Interface>
InterfaceRequest<Interface> MakeRequest(ScopedMessagePipeHandle handle) {
  InterfaceRequest<Interface> request;
  request.Bind(handle.Pass());
  return request;
}

// Creates a new message pipe over which Interface is to be served. Binds the
// specified InterfacePtr to one end of the message pipe, and returns an
// InterfaceRequest bound to the other. The InterfacePtr should be passed to
// the client, and the InterfaceRequest should be passed to whatever will
// provide the implementation. The implementation should typically be bound to
// the InterfaceRequest using the Binding or StrongBinding classes. The client
// may begin to issue calls even before an implementation has been bound, since
// messages sent over the pipe will just queue up until they are consumed by
// the implementation.
//
// Example #1: Requesting a remote implementation of an interface.
// ===============================================================
//
// Given the following interface:
//
//   interface Database {
//     OpenTable(Table& table);
//   }
//
// The client would have code similar to the following:
//
//   DatabasePtr database = ...;  // Connect to database.
//   TablePtr table;
//   database->OpenTable(GetProxy(&table));
//
// Upon return from GetProxy, |table| is ready to have methods called on it.
//
// Example #2: Registering a local implementation with a remote service.
// =====================================================================
//
// Given the following interface
//   interface Collector {
//     RegisterSource(Source source);
//   }
//
// The client would have code similar to the following:
//
//   CollectorPtr collector = ...;  // Connect to Collector.
//   SourcePtr source;
//   InterfaceRequest<Source> source_request = GetProxy(&source);
//   collector->RegisterSource(source.Pass());
//   CreateSource(source_request.Pass());  // Create implementation locally.
//
template <typename Interface>
InterfaceRequest<Interface> GetProxy(InterfacePtr<Interface>* ptr) {
  MessagePipe pipe;
  ptr->Bind(InterfaceHandle<Interface>(pipe.handle0.Pass(), 0u));
  return MakeRequest<Interface>(pipe.handle1.Pass());
}

// This is an overload of GetProxy() that assumes that the client end of the
// handle is an InterfaceHandle<>. It creates a new message pipe over which the
// Interface is to be served. InterfaceHandle<> represents the client's end,
// and InterfaceRequest<> should be used to bind to an implementation.
template <typename Interface>
InterfaceRequest<Interface> GetProxy(InterfaceHandle<Interface>* ptr) {
  MessagePipe pipe;
  *ptr = InterfaceHandle<Interface>(pipe.handle0.Pass(), 0u);
  return MakeRequest<Interface>(pipe.handle1.Pass());
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_REQUEST_H_
