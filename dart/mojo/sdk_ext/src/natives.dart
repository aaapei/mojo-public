// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of internal;

class MojoCoreNatives {
  static int getTimeTicksNow() native "Mojo_GetTimeTicksNow";
}

class MojoHandleNatives {
  static Set<int> _unclosedHandles = new Set<int>();

  static void addUnclosed(int handle) {
    _unclosedHandles.add(handle);
  }

  static void removeUnclosed(int handle) {
    _unclosedHandles.remove(handle);
  }

  static int registerFinalizer(Object eventStream, int handle)
      native "MojoHandle_RegisterFinalizer";

  static int close(int handle) native "MojoHandle_Close";

  static List wait(int handle, int signals, int deadline)
      native "MojoHandle_Wait";

  static List waitMany(List<int> handles, List<int> signals, int deadline)
      native "MojoHandle_WaitMany";

  // Called from the embedder's unhandled exception callback.
  // Returns the number of successfully closed handles.
  static int _closeUnclosedHandles() {
    int count = 0;
    _unclosedHandles.forEach((h) {
      if (MojoHandleNatives.close(h) == 0) {
        count++;
      }
    });
    _unclosedHandles.clear();
    return count;
  }
}

class MojoHandleWatcherNatives {
  static int sendControlData(
      int controlHandle, int mojoHandle, SendPort port, int data)
      native "MojoHandleWatcher_SendControlData";
  static List recvControlData(int controlHandle)
      native "MojoHandleWatcher_RecvControlData";
  static int setControlHandle(int controlHandle)
      native "MojoHandleWatcher_SetControlHandle";
  static int getControlHandle() native "MojoHandleWatcher_GetControlHandle";
  static List growStateArrays(int newLength)
      native "MojoHandleWatcher_GrowStateArrays";
  static int waitMany(int length, int deadline)
      native "MojoHandleWatcher_WaitMany";
}

class MojoMessagePipeNatives {
  static List MojoCreateMessagePipe(int flags) native "MojoMessagePipe_Create";

  static int MojoWriteMessage(
      int handle, ByteData data, int numBytes, List<int> handles, int flags)
      native "MojoMessagePipe_Write";

  static List MojoReadMessage(
      int handle, ByteData data, int numBytes, List<int> handles, int flags)
      native "MojoMessagePipe_Read";
}

class MojoDataPipeNatives {
  static List MojoCreateDataPipe(int elementBytes, int capacityBytes, int flags)
      native "MojoDataPipe_Create";

  static List MojoWriteData(int handle, ByteData data, int numBytes, int flags)
      native "MojoDataPipe_WriteData";

  static List MojoBeginWriteData(int handle, int bufferBytes, int flags)
      native "MojoDataPipe_BeginWriteData";

  static int MojoEndWriteData(int handle, int bytesWritten)
      native "MojoDataPipe_EndWriteData";

  static List MojoReadData(int handle, ByteData data, int numBytes, int flags)
      native "MojoDataPipe_ReadData";

  static List MojoBeginReadData(int handle, int bufferBytes, int flags)
      native "MojoDataPipe_BeginReadData";

  static int MojoEndReadData(int handle, int bytesRead)
      native "MojoDataPipe_EndReadData";
}

class MojoSharedBufferNatives {
  static List Create(int numBytes, int flags) native "MojoSharedBuffer_Create";

  static List Duplicate(int bufferHandle, int flags)
      native "MojoSharedBuffer_Duplicate";

  static List Map(
      Object buffer, int bufferHandle, int offset, int numBytes, int flags)
      native "MojoSharedBuffer_Map";

  static int Unmap(ByteData buffer) native "MojoSharedBuffer_Unmap";
}
