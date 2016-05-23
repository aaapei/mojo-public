// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_IMPL_PRIVATE_THUNKS_H_
#define MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_IMPL_PRIVATE_THUNKS_H_

#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/time.h"
#include "mojo/public/platform/native/system_impl_private.h"

// Structure used to bind the basic Mojo Core functions of a DSO to those of
// the embedder.
// This is the ABI between the embedder and the DSO. It can only have new
// functions added to the end. No other changes are supported.
#pragma pack(push, 8)
struct MojoSystemImplControlThunksPrivate {
  size_t size;  // Should be set to sizeof(MojoSystemImplThunks).
  MojoSystemImpl (*GetDefaultSystemImpl)(void);
  MojoSystemImpl (*CreateSystemImpl)(void);
  MojoResult (*TransferHandle)(MojoSystemImpl from_system,
                               MojoHandle handle,
                               MojoSystemImpl to_system,
                               MojoHandle* result_handle);
};

struct MojoSystemImplThunksPrivate {
  size_t size;  // Should be set to sizeof(MojoExplicitThunksPrivate).
  MojoTimeTicks (*GetTimeTicksNow)(MojoSystemImpl system);
  MojoResult (*Close)(MojoSystemImpl system, MojoHandle handle);
  MojoResult (*GetRights)(MojoSystemImpl system,
                          MojoHandle handle,
                          MojoHandleRights* rights);
  MojoResult (*DuplicateHandleWithReducedRights)(
      MojoSystemImpl system,
      MojoHandle handle,
      MojoHandleRights rights_to_remove,
      MojoHandle* new_handle);
  MojoResult (*DuplicateHandle)(MojoSystemImpl system,
                                MojoHandle handle,
                                MojoHandle* new_handle);
  MojoResult (*Wait)(MojoSystemImpl system,
                     MojoHandle handle,
                     MojoHandleSignals signals,
                     MojoDeadline deadline,
                     struct MojoHandleSignalsState* signals_state);
  MojoResult (*WaitMany)(MojoSystemImpl system,
                         const MojoHandle* handles,
                         const MojoHandleSignals* signals,
                         uint32_t num_handles,
                         MojoDeadline deadline,
                         uint32_t* result_index,
                         struct MojoHandleSignalsState* signals_states);
  MojoResult (*CreateMessagePipe)(
      MojoSystemImpl system,
      const struct MojoCreateMessagePipeOptions* options,
      MojoHandle* message_pipe_handle0,
      MojoHandle* message_pipe_handle1);
  MojoResult (*WriteMessage)(MojoSystemImpl system,
                             MojoHandle message_pipe_handle,
                             const void* bytes,
                             uint32_t num_bytes,
                             const MojoHandle* handles,
                             uint32_t num_handles,
                             MojoWriteMessageFlags flags);
  MojoResult (*ReadMessage)(MojoSystemImpl system,
                            MojoHandle message_pipe_handle,
                            void* bytes,
                            uint32_t* num_bytes,
                            MojoHandle* handles,
                            uint32_t* num_handles,
                            MojoReadMessageFlags flags);
  MojoResult (*CreateDataPipe)(MojoSystemImpl system,
                               const struct MojoCreateDataPipeOptions* options,
                               MojoHandle* data_pipe_producer_handle,
                               MojoHandle* data_pipe_consumer_handle);
  MojoResult (*SetDataPipeProducerOptions)(
      MojoSystemImpl system,
      MojoHandle data_pipe_producer_handle,
      const struct MojoDataPipeProducerOptions* options);
  MojoResult (*GetDataPipeProducerOptions)(
      MojoSystemImpl system,
      MojoHandle data_pipe_producer_handle,
      struct MojoDataPipeProducerOptions* options,
      uint32_t options_num_bytes);
  MojoResult (*WriteData)(MojoSystemImpl system,
                          MojoHandle data_pipe_producer_handle,
                          const void* elements,
                          uint32_t* num_elements,
                          MojoWriteDataFlags flags);
  MojoResult (*BeginWriteData)(MojoSystemImpl system,
                               MojoHandle data_pipe_producer_handle,
                               void** buffer,
                               uint32_t* buffer_num_elements,
                               MojoWriteDataFlags flags);
  MojoResult (*EndWriteData)(MojoSystemImpl system,
                             MojoHandle data_pipe_producer_handle,
                             uint32_t num_elements_written);
  MojoResult (*SetDataPipeConsumerOptions)(
      MojoSystemImpl system,
      MojoHandle data_pipe_consumer_handle,
      const struct MojoDataPipeConsumerOptions* options);
  MojoResult (*GetDataPipeConsumerOptions)(
      MojoSystemImpl system,
      MojoHandle data_pipe_consumer_handle,
      struct MojoDataPipeConsumerOptions* options,
      uint32_t options_num_bytes);
  MojoResult (*ReadData)(MojoSystemImpl system,
                         MojoHandle data_pipe_consumer_handle,
                         void* elements,
                         uint32_t* num_elements,
                         MojoReadDataFlags flags);
  MojoResult (*BeginReadData)(MojoSystemImpl system,
                              MojoHandle data_pipe_consumer_handle,
                              const void** buffer,
                              uint32_t* buffer_num_elements,
                              MojoReadDataFlags flags);
  MojoResult (*EndReadData)(MojoSystemImpl system,
                            MojoHandle data_pipe_consumer_handle,
                            uint32_t num_elements_read);
  MojoResult (*CreateSharedBuffer)(
      MojoSystemImpl system,
      const struct MojoCreateSharedBufferOptions* options,
      uint64_t num_bytes,
      MojoHandle* shared_buffer_handle);
  MojoResult (*DuplicateBufferHandle)(
      MojoSystemImpl system,
      MojoHandle buffer_handle,
      const struct MojoDuplicateBufferHandleOptions* options,
      MojoHandle* new_buffer_handle);
  MojoResult (*GetBufferInformation)(MojoSystemImpl system,
                                     MojoHandle buffer_handle,
                                     struct MojoBufferInformation* info,
                                     uint32_t info_num_bytes);
  MojoResult (*MapBuffer)(MojoSystemImpl system,
                          MojoHandle buffer_handle,
                          uint64_t offset,
                          uint64_t num_bytes,
                          void** buffer,
                          MojoMapBufferFlags flags);
  MojoResult (*UnmapBuffer)(MojoSystemImpl system, void* buffer);
};
#pragma pack(pop)

#ifdef __cplusplus
inline MojoSystemImplControlThunksPrivate
MojoMakeSystemImplControlThunksPrivate() {
  MojoSystemImplControlThunksPrivate system_thunks = {
      sizeof(MojoSystemImplControlThunksPrivate),
      MojoSystemImplGetDefaultImpl,
      MojoSystemImplCreateImpl,
      MojoSystemImplTransferHandle};
  return system_thunks;
}

inline MojoSystemImplThunksPrivate MojoMakeSystemImplThunksPrivate() {
  MojoSystemImplThunksPrivate system_thunks = {
      sizeof(MojoSystemImplThunksPrivate),
      MojoSystemImplGetTimeTicksNow,
      MojoSystemImplClose,
      MojoSystemImplGetRights,
      MojoSystemImplDuplicateHandleWithReducedRights,
      MojoSystemImplDuplicateHandle,
      MojoSystemImplWait,
      MojoSystemImplWaitMany,
      MojoSystemImplCreateMessagePipe,
      MojoSystemImplWriteMessage,
      MojoSystemImplReadMessage,
      MojoSystemImplCreateDataPipe,
      MojoSystemImplSetDataPipeProducerOptions,
      MojoSystemImplGetDataPipeProducerOptions,
      MojoSystemImplWriteData,
      MojoSystemImplBeginWriteData,
      MojoSystemImplEndWriteData,
      MojoSystemImplSetDataPipeConsumerOptions,
      MojoSystemImplGetDataPipeConsumerOptions,
      MojoSystemImplReadData,
      MojoSystemImplBeginReadData,
      MojoSystemImplEndReadData,
      MojoSystemImplCreateSharedBuffer,
      MojoSystemImplDuplicateBufferHandle,
      MojoSystemImplGetBufferInformation,
      MojoSystemImplMapBuffer,
      MojoSystemImplUnmapBuffer};
  return system_thunks;
}
#endif

#endif  // MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_IMPL_PRIVATE_THUNKS_H_
