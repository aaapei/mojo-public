// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package bindings

import (
	"sync"
)

// MessageReceiver can receive |Message| objects.
type MessageReceiver interface {
	// Accept receives a |Message|. Returns a error if the message was not
	// handled.
	Accept(message *Message) error
}

// Stub is a base implementation of Stub. Stubs receives messages from message
// pipe, deserialize the payload and call the appropriate method in the
// implementation. If the method returns result, the stub serializes the
// response and sends it back.
type Stub struct {
	// Makes sure that connector is closed only once.
	closeOnce sync.Once
	connector *Connector
	receiver  MessageReceiver
}

// NewStub returns a new Stub instance using provided |Connector| to send and
// receive messages. Incoming messages are handled by the provided |receiver|.
func NewStub(connector *Connector, receiver MessageReceiver) *Stub {
	return &Stub{
		connector: connector,
		receiver:  receiver,
	}
}

// ServeRequest synchronously serves one request from the message pipe: the
// |Stub| waits on its underlying message pipe for a message and handles it.
// Can be called from multiple goroutines. Each calling goroutine will receive
// a different message or an error. Can be called concurrently with
// ServeRequestInGoroutine. Closes itself in case of a message pipe error or
// a non-nil error returned by the MessageReceiver.
func (s *Stub) ServeRequest() error {
	message, err := s.connector.ReadMessage()
	if err != nil {
		s.Close()
		return err
	}
	err = s.receiver.Accept(message)
	if err != nil {
		s.Close()
	}
	return err
}

// ServeRequestInGoroutine is version of ServeRequest that spawns a goroutine to
// handle the incoming message.
// The |Stub| waits on its underlying message pipe for a message, starts a
// goroutine to handle the message and returns a channel. There will be sent
// exactly one object through the returned channel: an encountered error or nil.
// ServeRequestInGoroutine can be called from multiple goroutines. Each calling
// goroutine will receive a different message or an error. Can be called
// concurrently with ServeRequest. Closes itself in case of a message pipe error
// or a non-nil error returned by the MessageReceiver.
func (s *Stub) ServeRequestInGoroutine() <-chan error {
	message, err := s.connector.ReadMessage()
	c := make(chan error, 1)
	if err != nil {
		s.Close()
		c <- err
		return c
	}
	go func() {
		err := s.receiver.Accept(message)
		if err != nil {
			s.Close()
		}
		c <- err
	}()
	return c
}

// Close immediately closes the |Stub| and its underlying message pipe. If the
// |Stub| is waiting on its message pipe handle the wait process is interrupted.
// All goroutines trying to serve will start returning errors as the underlying
// message pipe becomes invalid.
func (s *Stub) Close() {
	s.closeOnce.Do(func() {
		s.connector.Close()
	})
}
