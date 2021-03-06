// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_THUNKS_H_
#define MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_THUNKS_H_

#include <mojo/result.h>
#include <mojo/system/buffer.h>
#include <mojo/system/data_pipe.h>
#include <mojo/system/handle.h>
#include <mojo/system/message_pipe.h>
#include <mojo/system/time.h>
#include <mojo/system/wait.h>
#include <mojo/system/wait_set.h>
#include <stddef.h>

// The embedder needs to bind the basic Mojo Core functions of a DSO to those of
// the embedder when loading a DSO that is dependent on mojo_system.
// The typical usage would look like:
// base::ScopedNativeLibrary app_library(
//     base::LoadNativeLibrary(app_path_, &error));
// typedef MojoResult (*MojoSetSystemThunksFn)(MojoSystemThunks*);
// MojoSetSystemThunksFn mojo_set_system_thunks_fn =
//     reinterpret_cast<MojoSetSystemThunksFn>(app_library.GetFunctionPointer(
//         "MojoSetSystemThunks"));
// MojoSystemThunks system_thunks = MojoMakeSystemThunks();
// size_t expected_size = mojo_set_system_thunks_fn(&system_thunks);
// if (expected_size > sizeof(MojoSystemThunks)) {
//   LOG(ERROR)
//       << "Invalid DSO. Expected MojoSystemThunks size: "
//       << expected_size;
//   break;
// }

// Structure used to bind the basic Mojo Core functions of a DSO to those of
// the embedder.
// This is the ABI between the embedder and the DSO. It can only have new
// functions added to the end. No other changes are supported.
#pragma pack(push, 8)
struct MojoSystemThunks {
  size_t size;  // Should be set to sizeof(MojoSystemThunks).
  MojoTimeTicks (*GetTimeTicksNow)();
  MojoResult (*Close)(MojoHandle handle);
  MojoResult (*Wait)(MojoHandle handle,
                     MojoHandleSignals signals,
                     MojoDeadline deadline,
                     struct MojoHandleSignalsState* signals_state);
  MojoResult (*WaitMany)(const MojoHandle* handles,
                         const MojoHandleSignals* signals,
                         uint32_t num_handles,
                         MojoDeadline deadline,
                         uint32_t* result_index,
                         struct MojoHandleSignalsState* signals_states);
  MojoResult (*CreateMessagePipe)(
      const struct MojoCreateMessagePipeOptions* options,
      MojoHandle* message_pipe_handle0,
      MojoHandle* message_pipe_handle1);
  MojoResult (*WriteMessage)(MojoHandle message_pipe_handle,
                             const void* bytes,
                             uint32_t num_bytes,
                             const MojoHandle* handles,
                             uint32_t num_handles,
                             MojoWriteMessageFlags flags);
  MojoResult (*ReadMessage)(MojoHandle message_pipe_handle,
                            void* bytes,
                            uint32_t* num_bytes,
                            MojoHandle* handles,
                            uint32_t* num_handles,
                            MojoReadMessageFlags flags);
  MojoResult (*CreateDataPipe)(const struct MojoCreateDataPipeOptions* options,
                               MojoHandle* data_pipe_producer_handle,
                               MojoHandle* data_pipe_consumer_handle);
  MojoResult (*WriteData)(MojoHandle data_pipe_producer_handle,
                          const void* elements,
                          uint32_t* num_elements,
                          MojoWriteDataFlags flags);
  MojoResult (*BeginWriteData)(MojoHandle data_pipe_producer_handle,
                               void** buffer,
                               uint32_t* buffer_num_elements,
                               MojoWriteDataFlags flags);
  MojoResult (*EndWriteData)(MojoHandle data_pipe_producer_handle,
                             uint32_t num_elements_written);
  MojoResult (*ReadData)(MojoHandle data_pipe_consumer_handle,
                         void* elements,
                         uint32_t* num_elements,
                         MojoReadDataFlags flags);
  MojoResult (*BeginReadData)(MojoHandle data_pipe_consumer_handle,
                              const void** buffer,
                              uint32_t* buffer_num_elements,
                              MojoReadDataFlags flags);
  MojoResult (*EndReadData)(MojoHandle data_pipe_consumer_handle,
                            uint32_t num_elements_read);
  MojoResult (*CreateSharedBuffer)(
      const struct MojoCreateSharedBufferOptions* options,
      uint64_t num_bytes,
      MojoHandle* shared_buffer_handle);
  MojoResult (*DuplicateBufferHandle)(
      MojoHandle buffer_handle,
      const struct MojoDuplicateBufferHandleOptions* options,
      MojoHandle* new_buffer_handle);
  MojoResult (*MapBuffer)(MojoHandle buffer_handle,
                          uint64_t offset,
                          uint64_t num_bytes,
                          void** buffer,
                          MojoMapBufferFlags flags);
  MojoResult (*UnmapBuffer)(void* buffer);
  // TODO(vtl): The next time an ABI-breaking change is made, re-order the
  // following elements (to match the order in |Core|).
  MojoResult (*GetBufferInformation)(MojoHandle buffer_handle,
                                     struct MojoBufferInformation* info,
                                     uint32_t info_num_bytes);
  MojoResult (*SetDataPipeConsumerOptions)(
      MojoHandle data_pipe_consumer_handle,
      const struct MojoDataPipeConsumerOptions* options);
  MojoResult (*GetDataPipeConsumerOptions)(
      MojoHandle data_pipe_consumer_handle,
      struct MojoDataPipeConsumerOptions* options,
      uint32_t options_num_bytes);
  MojoResult (*SetDataPipeProducerOptions)(
      MojoHandle data_pipe_producer_handle,
      const struct MojoDataPipeProducerOptions* options);
  MojoResult (*GetDataPipeProducerOptions)(
      MojoHandle data_pipe_producer_handle,
      struct MojoDataPipeProducerOptions* options,
      uint32_t options_num_bytes);
  MojoResult (*GetRights)(MojoHandle handle, MojoHandleRights* rights);
  MojoResult (*DuplicateHandleWithReducedRights)(
      MojoHandle handle,
      MojoHandleRights rights_to_remove,
      MojoHandle* new_handle);
  MojoResult (*DuplicateHandle)(MojoHandle handle, MojoHandle* new_handle);
  MojoResult (*ReplaceHandleWithReducedRights)(
      MojoHandle handle,
      MojoHandleRights rights_to_remove,
      MojoHandle* replacement_handle);
  MojoResult (*CreateWaitSet)(const struct MojoCreateWaitSetOptions* options,
                              MojoHandle* handle);
  MojoResult (*WaitSetAdd)(MojoHandle wait_set_handle,
                           MojoHandle handle,
                           MojoHandleSignals signals,
                           uint64_t cookie,
                           const struct MojoWaitSetAddOptions* options);
  MojoResult (*WaitSetRemove)(MojoHandle wait_set_handle, uint64_t cookie);
  MojoResult (*WaitSetWait)(MojoHandle wait_set_handle,
                            MojoDeadline deadline,
                            uint32_t* num_results,
                            struct MojoWaitSetResult* results,
                            uint32_t* max_results);
};
#pragma pack(pop)

#ifdef __cplusplus
// Intended to be called from the embedder. Returns a |MojoCore| initialized
// to contain pointers to each of the embedder's MojoCore functions.
inline MojoSystemThunks MojoMakeSystemThunks() {
  MojoSystemThunks system_thunks = {
      sizeof(MojoSystemThunks),
      MojoGetTimeTicksNow,
      MojoClose,
      MojoWait,
      MojoWaitMany,
      MojoCreateMessagePipe,
      MojoWriteMessage,
      MojoReadMessage,
      MojoCreateDataPipe,
      MojoWriteData,
      MojoBeginWriteData,
      MojoEndWriteData,
      MojoReadData,
      MojoBeginReadData,
      MojoEndReadData,
      MojoCreateSharedBuffer,
      MojoDuplicateBufferHandle,
      MojoMapBuffer,
      MojoUnmapBuffer,
      MojoGetBufferInformation,
      MojoSetDataPipeConsumerOptions,
      MojoGetDataPipeConsumerOptions,
      MojoSetDataPipeProducerOptions,
      MojoGetDataPipeProducerOptions,
      MojoGetRights,
      MojoDuplicateHandleWithReducedRights,
      MojoDuplicateHandle,
      MojoReplaceHandleWithReducedRights,
      MojoCreateWaitSet,
      MojoWaitSetAdd,
      MojoWaitSetRemove,
      MojoWaitSetWait,
  };
  return system_thunks;
}
#endif


// Use this type for the function found by dynamically discovering it in
// a DSO linked with mojo_system. For example:
// MojoSetSystemThunksFn mojo_set_system_thunks_fn =
//     reinterpret_cast<MojoSetSystemThunksFn>(app_library.GetFunctionPointer(
//         "MojoSetSystemThunks"));
// The expected size of |system_thunks} is returned.
// The contents of |system_thunks| are copied.
typedef size_t (*MojoSetSystemThunksFn)(
    const struct MojoSystemThunks* system_thunks);

#endif  // MOJO_PUBLIC_PLATFORM_NATIVE_SYSTEM_THUNKS_H_
