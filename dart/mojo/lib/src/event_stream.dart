// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of core;

class MojoEventStream extends Stream<List<int>> {
  // The underlying Mojo handle.
  MojoHandle _handle;

  // Providing our own stream controller allows us to take custom actions when
  // listeners pause/resume/etc. their StreamSubscription.
  StreamController _controller;

  // The send port that we give to the handle watcher to notify us of handle
  // events.
  SendPort _sendPort;

  // The receive port on which we listen and receive events from the handle
  // watcher.
  ReceivePort _receivePort;

  // The signals on this handle that we're interested in.
  MojoHandleSignals _signals;

  // Whether listen has been called.
  bool _isListening;

  MojoEventStream(MojoHandle handle,
      [MojoHandleSignals signals = MojoHandleSignals.PEER_CLOSED_READABLE])
      : _handle = handle,
        _signals = signals,
        _isListening = false {
    MojoResult result = MojoHandle.registerFinalizer(this);
    if (!result.isOk) {
      throw new MojoInternalError(
          "Failed to register the MojoHandle: $result.");
    }
  }

  Future close({bool immediate: false}) => _close(immediate: immediate);

  StreamSubscription<List<int>> listen(void onData(List event),
      {Function onError, void onDone(), bool cancelOnError}) {
    if (_isListening) {
      throw new MojoApiError("Listen has already been called: $_handle.");
    }
    _receivePort = new ReceivePort();
    _sendPort = _receivePort.sendPort;
    _controller = new StreamController(
        sync: true,
        onPause: _onPauseStateChange,
        onResume: _onPauseStateChange);
    _controller.addStream(_receivePort).whenComplete(_controller.close);

    if (_signals != MojoHandleSignals.NONE) {
      var res = new MojoResult(
          MojoHandleWatcher.add(_handle.h, _sendPort, _signals.value));
      if (!res.isOk) {
        throw new MojoInternalError("MojoHandleWatcher add failed: $res");
      }
    }

    _isListening = true;
    return _controller.stream.listen(onData,
        onError: onError, onDone: onDone, cancelOnError: cancelOnError);
  }

  bool enableSignals(MojoHandleSignals signals) {
    _signals = signals;
    if (_isListening) {
      return MojoHandleWatcher.add(_handle.h, _sendPort, signals.value) ==
          MojoResult.kOk;
    }
    return false;
  }

  bool enableReadEvents() =>
      enableSignals(MojoHandleSignals.PEER_CLOSED_READABLE);
  bool enableWriteEvents() => enableSignals(MojoHandleSignals.WRITABLE);
  bool enableAllEvents() => enableSignals(MojoHandleSignals.READWRITE);

  Future _close({bool immediate: false, bool local: false}) {
    if (_handle != null) {
      if (_isListening && !local) {
        return _handleWatcherClose(immediate: immediate).then((result) {
          // If the handle watcher is gone, then close the handle ourselves.
          if (!result.isOk) {
            _localClose();
          }
        });
      } else {
        _localClose();
      }
    }
    return new Future.value(null);
  }

  Future _handleWatcherClose({bool immediate: false}) {
    assert(MojoHandle._removeUnclosedHandle(_handle));
    MojoHandleNatives.removeUnclosed(_handle.h);
    return MojoHandleWatcher.close(_handle.h, wait: !immediate).then((r) {
      if (_receivePort != null) {
        _receivePort.close();
        _receivePort = null;
      }
      return new MojoResult(r);
    });
  }

  void _localClose() {
    _handle.close();
    _handle = null;
    if (_receivePort != null) {
      _receivePort.close();
      _receivePort = null;
    }
  }

  void _onPauseStateChange() {
    if (_controller.isPaused) {
      var res = new MojoResult(MojoHandleWatcher.remove(_handle.h));
      if (!res.isOk) {
        throw new MojoInternalError("MojoHandleWatcher add failed: $res");
      }
    } else {
      var res = new MojoResult(
          MojoHandleWatcher.add(_handle.h, _sendPort, _signals.value));
      if (!res.isOk) {
        throw new MojoInternalError("MojoHandleWatcher add failed: $res");
      }
    }
  }

  bool get readyRead => _handle.readyRead;
  bool get readyWrite => _handle.readyWrite;

  String toString() => "$_handle";
}

typedef void ErrorHandler();

class MojoEventStreamListener {
  StreamSubscription subscription;
  ErrorHandler onError;

  MojoMessagePipeEndpoint _endpoint;
  MojoEventStream _eventStream;
  bool _isOpen = false;
  bool _isInHandler = false;
  bool _isPeerClosed = false;

  MojoEventStreamListener.fromEndpoint(MojoMessagePipeEndpoint endpoint)
      : _endpoint = endpoint,
        _eventStream = new MojoEventStream(endpoint.handle) {
    listen();
  }

  MojoEventStreamListener.fromHandle(MojoHandle handle) {
    _endpoint = new MojoMessagePipeEndpoint(handle);
    _eventStream = new MojoEventStream(handle);
    listen();
  }

  MojoEventStreamListener.unbound();

  void bind(MojoMessagePipeEndpoint endpoint) {
    if (isBound) {
      throw new MojoApiError("MojoEventStreamListener is already bound.");
    }
    _endpoint = endpoint;
    _eventStream = new MojoEventStream(endpoint.handle);
    _isOpen = false;
    _isInHandler = false;
    _isPeerClosed = false;
  }

  void bindFromHandle(MojoHandle handle) {
    if (isBound) {
      throw new MojoApiError("MojoEventStreamListener is already bound.");
    }
    _endpoint = new MojoMessagePipeEndpoint(handle);
    _eventStream = new MojoEventStream(handle);
    _isOpen = false;
    _isInHandler = false;
    _isPeerClosed = false;
  }

  StreamSubscription<List<int>> listen() {
    if (!isBound) {
      throw new MojoApiError("MojoEventStreamListener is unbound.");
    }
    if (subscription != null) {
      throw new MojoApiError("Listen has already been called.");
    }
    _isOpen = true;
    subscription = _eventStream.listen((List<int> event) {
      if (!_isOpen) {
        // The actual close of the underlying stream happens asynchronously
        // after the call to close. However, we start to ignore incoming events
        // immediately.
        return;
      }
      var signalsWatched = new MojoHandleSignals(event[0]);
      var signalsReceived = new MojoHandleSignals(event[1]);
      _isInHandler = true;
      if (signalsReceived.isReadable) {
        assert(_eventStream.readyRead);
        handleRead();
      }
      if (signalsReceived.isWritable) {
        assert(_eventStream.readyWrite);
        handleWrite();
      }
      _isPeerClosed = signalsReceived.isPeerClosed ||
          !_eventStream.enableSignals(signalsWatched);
      _isInHandler = false;
      if (_isPeerClosed) {
        close().then((_) {
          if (onError != null) {
            onError();
          }
        });
      }
    }, onDone: close);
    return subscription;
  }

  Future close({bool immediate: false}) {
    var result;
    _isOpen = false;
    _endpoint = null;
    subscription = null;
    if (_eventStream != null) {
      result = _eventStream
          ._close(immediate: immediate, local: _isPeerClosed)
          .then((_) {
        _eventStream = null;
      });
    }
    return result != null ? result : new Future.value(null);
  }

  void handleRead() {}
  void handleWrite() {}

  MojoMessagePipeEndpoint get endpoint => _endpoint;
  bool get isOpen => _isOpen;
  bool get isInHandler => _isInHandler;
  bool get isBound => _endpoint != null;
  bool get isPeerClosed => _isPeerClosed;

  String toString() => "MojoEventStreamListener("
      "isOpen: $isOpen, isBound: $isBound, endpoint: $_endpoint)";
}
