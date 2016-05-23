// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_IMPL_PRIVATE_H_
#define MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_IMPL_PRIVATE_H_

#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/time.h"

// This interface provides the Mojo system API, but with the ability to confine
// calls to a specific handle namespace. Handles in one namespace are unrelated
// to handles in another namespace. Two ends of a pipe may live in different
// handle namespaces, however.

typedef void* MojoSystemImpl;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// APIs for creating and manipulating MojoSystemImpls.

// Returns the MojoSystemImpl implicitly used by the non-SystemImpl version of
// the Mojo sytem APIs.
MojoSystemImpl MojoSystemImplGetDefaultImpl();

// Creates and returns a new MojoSystemImpl. Currently there is no way to
// destroy a MojoSystemImpl, once created.
MojoSystemImpl MojoSystemImplCreateImpl();

// Moves a handle from one MojoSystemImpl to another.
// On success, |result_handle| contains the name of the handle in the new
// namespace.
// If |MOJO_RESULT_RESOURCE_EXHAUSTED| is returned, the |handle| will have been
// closed, and is now lost.
// Busy handles cannot be transfered.
// To avoid trouble, this API should only be used to bootstrap a newly created
// |to_system| with a newly created |handle|.
MojoResult MojoSystemImplTransferHandle(MojoSystemImpl from_system,
                                        MojoHandle handle,
                                        MojoSystemImpl to_system,
                                        MojoHandle* result_handle);

// APIs mirroring the Mojo system APIs, but also taking a MojoSystemImpl param.
MojoTimeTicks MojoSystemImplGetTimeTicksNow(MojoSystemImpl system);
MojoResult MojoSystemImplClose(MojoSystemImpl system, MojoHandle handle);
MojoResult MojoSystemImplGetRights(MojoSystemImpl system,
                                   MojoHandle handle,
                                   MojoHandleRights* rights);
MojoResult MojoSystemImplDuplicateHandleWithReducedRights(
    MojoSystemImpl system,
    MojoHandle handle,
    MojoHandleRights rights_to_remove,
    MojoHandle* new_handle);
MojoResult MojoSystemImplDuplicateHandle(MojoSystemImpl system,
                                         MojoHandle handle,
                                         MojoHandle* new_handle);
MojoResult MojoSystemImplWait(MojoSystemImpl system,
                              MojoHandle handle,
                              MojoHandleSignals signals,
                              MojoDeadline deadline,
                              struct MojoHandleSignalsState* signals_state);
MojoResult MojoSystemImplWaitMany(
    MojoSystemImpl system,
    const MojoHandle* handles,
    const MojoHandleSignals* signals,
    uint32_t num_handles,
    MojoDeadline deadline,
    uint32_t* result_index,
    struct MojoHandleSignalsState* signals_states);
MojoResult MojoSystemImplCreateMessagePipe(
    MojoSystemImpl system,
    const struct MojoCreateMessagePipeOptions* options,
    MojoHandle* message_pipe_handle0,
    MojoHandle* message_pipe_handle1);
MojoResult MojoSystemImplWriteMessage(MojoSystemImpl system,
                                      MojoHandle message_pipe_handle,
                                      const void* bytes,
                                      uint32_t num_bytes,
                                      const MojoHandle* handles,
                                      uint32_t num_handles,
                                      MojoWriteMessageFlags flags);
MojoResult MojoSystemImplReadMessage(MojoSystemImpl system,
                                     MojoHandle message_pipe_handle,
                                     void* bytes,
                                     uint32_t* num_bytes,
                                     MojoHandle* handles,
                                     uint32_t* num_handles,
                                     MojoReadMessageFlags flags);
MojoResult MojoSystemImplCreateDataPipe(
    MojoSystemImpl system,
    const struct MojoCreateDataPipeOptions* options,
    MojoHandle* data_pipe_producer_handle,
    MojoHandle* data_pipe_consumer_handle);
MojoResult MojoSystemImplSetDataPipeProducerOptions(
    MojoSystemImpl system,
    MojoHandle data_pipe_producer_handle,
    const struct MojoDataPipeProducerOptions* options);
MojoResult MojoSystemImplGetDataPipeProducerOptions(
    MojoSystemImpl system,
    MojoHandle data_pipe_producer_handle,
    struct MojoDataPipeProducerOptions* options,
    uint32_t options_num_bytes);
MojoResult MojoSystemImplWriteData(MojoSystemImpl system,
                                   MojoHandle data_pipe_producer_handle,
                                   const void* elements,
                                   uint32_t* num_elements,
                                   MojoWriteDataFlags flags);
MojoResult MojoSystemImplBeginWriteData(MojoSystemImpl system,
                                        MojoHandle data_pipe_producer_handle,
                                        void** buffer,
                                        uint32_t* buffer_num_elements,
                                        MojoWriteDataFlags flags);
MojoResult MojoSystemImplEndWriteData(MojoSystemImpl system,
                                      MojoHandle data_pipe_producer_handle,
                                      uint32_t num_elements_written);
MojoResult MojoSystemImplSetDataPipeConsumerOptions(
    MojoSystemImpl system,
    MojoHandle data_pipe_consumer_handle,
    const struct MojoDataPipeConsumerOptions* options);
MojoResult MojoSystemImplGetDataPipeConsumerOptions(
    MojoSystemImpl system,
    MojoHandle data_pipe_consumer_handle,
    struct MojoDataPipeConsumerOptions* options,
    uint32_t options_num_bytes);
MojoResult MojoSystemImplReadData(MojoSystemImpl system,
                                  MojoHandle data_pipe_consumer_handle,
                                  void* elements,
                                  uint32_t* num_elements,
                                  MojoReadDataFlags flags);
MojoResult MojoSystemImplBeginReadData(MojoSystemImpl system,
                                       MojoHandle data_pipe_consumer_handle,
                                       const void** buffer,
                                       uint32_t* buffer_num_elements,
                                       MojoReadDataFlags flags);
MojoResult MojoSystemImplEndReadData(MojoSystemImpl system,
                                     MojoHandle data_pipe_consumer_handle,
                                     uint32_t num_elements_read);
MojoResult MojoSystemImplCreateSharedBuffer(
    MojoSystemImpl system,
    const struct MojoCreateSharedBufferOptions* options,
    uint64_t num_bytes,
    MojoHandle* shared_buffer_handle);
MojoResult MojoSystemImplDuplicateBufferHandle(
    MojoSystemImpl system,
    MojoHandle buffer_handle,
    const struct MojoDuplicateBufferHandleOptions* options,
    MojoHandle* new_buffer_handle);
MojoResult MojoSystemImplGetBufferInformation(
    MojoSystemImpl system,
    MojoHandle buffer_handle,
    struct MojoBufferInformation* info,
    uint32_t info_num_bytes);
MojoResult MojoSystemImplMapBuffer(MojoSystemImpl system,
                                   MojoHandle buffer_handle,
                                   uint64_t offset,
                                   uint64_t num_bytes,
                                   void** buffer,
                                   MojoMapBufferFlags flags);
MojoResult MojoSystemImplUnmapBuffer(MojoSystemImpl system, void* buffer);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_IMPL_PRIVATE_H_
