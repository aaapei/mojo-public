// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C data pipe API (the functions declared in
// mojo/public/c/include/mojo/system/data_pipe.h).

#include <mojo/system/data_pipe.h>

#include <mojo/system/handle.h>
#include <mojo/system/result.h>
#include <mojo/system/wait.h>
#include <string.h>

#include "gtest/gtest.h"
#include "mojo/public/cpp/system/macros.h"

namespace {

constexpr uint32_t kSizeOfOptions =
    static_cast<uint32_t>(sizeof(MojoCreateDataPipeOptions));

constexpr MojoHandleRights kDefaultDataPipeProducerHandleRights =
    MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_WRITE |
    MOJO_HANDLE_RIGHT_GET_OPTIONS | MOJO_HANDLE_RIGHT_SET_OPTIONS;
constexpr MojoHandleRights kDefaultDataPipeConsumerHandleRights =
    MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_READ |
    MOJO_HANDLE_RIGHT_GET_OPTIONS | MOJO_HANDLE_RIGHT_SET_OPTIONS;

constexpr MojoDeadline kZeroTimeout = 0u;
constexpr MojoDeadline kTinyTimeout = static_cast<MojoDeadline>(100u * 1000u);
constexpr MojoDeadline kActionTimeout =
    static_cast<MojoDeadline>(10u * 1000u * 1000u);

// Helper for testing *successful* data pipe creates.
void Create(const MojoCreateDataPipeOptions* options,
            MojoHandle* data_pipe_producer_handle,
            MojoHandle* data_pipe_consumer_handle) {
  *data_pipe_producer_handle = MOJO_HANDLE_INVALID;
  *data_pipe_consumer_handle = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoCreateDataPipe(options, data_pipe_producer_handle,
                               data_pipe_consumer_handle));
  EXPECT_NE(*data_pipe_producer_handle, MOJO_HANDLE_INVALID);
  EXPECT_NE(*data_pipe_consumer_handle, MOJO_HANDLE_INVALID);
  EXPECT_NE(*data_pipe_producer_handle, *data_pipe_consumer_handle);
}

// Helper for waiting for a certain amount of data. (Note: It resets the read
// threshold to the default when it's done.)
void WaitForData(MojoHandle hc, uint32_t num_bytes, MojoDeadline timeout) {
  MojoDataPipeConsumerOptions options = {
      static_cast<uint32_t>(sizeof(MojoDataPipeConsumerOptions)), num_bytes};
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeConsumerOptions(hc, &options));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, timeout, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeConsumerOptions(hc, nullptr));
}

// Helper for waiting for a certain amount of space. (Note: It resets the write
// threshold to the default when it's done.)
void WaitForSpace(MojoHandle hp, uint32_t num_bytes, MojoDeadline timeout) {
  MojoDataPipeProducerOptions options = {
      static_cast<uint32_t>(sizeof(MojoDataPipeProducerOptions)), num_bytes};
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeProducerOptions(hp, &options));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, timeout, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeProducerOptions(hp, nullptr));
}

TEST(DataPipeTest, InvalidHandle) {
  MojoDataPipeProducerOptions dpp_options = {
      sizeof(MojoDataPipeProducerOptions), 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoSetDataPipeProducerOptions(MOJO_HANDLE_INVALID, &dpp_options));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoGetDataPipeProducerOptions(
                MOJO_HANDLE_INVALID, &dpp_options,
                static_cast<uint32_t>(sizeof(dpp_options))));
  char buffer[10] = {};
  uint32_t buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoWriteData(MOJO_HANDLE_INVALID, buffer, &buffer_size,
                          MOJO_WRITE_DATA_FLAG_NONE));
  void* write_pointer = nullptr;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoBeginWriteData(MOJO_HANDLE_INVALID, &write_pointer,
                               &buffer_size, MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoEndWriteData(MOJO_HANDLE_INVALID, 1u));
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  MojoDataPipeConsumerOptions dpc_options = {
      sizeof(MojoDataPipeConsumerOptions), 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoSetDataPipeConsumerOptions(MOJO_HANDLE_INVALID, &dpc_options));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoGetDataPipeConsumerOptions(
                MOJO_HANDLE_INVALID, &dpc_options,
                static_cast<uint32_t>(sizeof(dpc_options))));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoReadData(MOJO_HANDLE_INVALID, buffer, &buffer_size,
                         MOJO_READ_DATA_FLAG_NONE));
  const void* read_pointer = nullptr;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoBeginReadData(MOJO_HANDLE_INVALID, &read_pointer, &buffer_size,
                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoEndReadData(MOJO_HANDLE_INVALID, 1u));
}

TEST(DataPipeTest, Basic) {
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(nullptr, &hp, &hc);

  // Both handles should have the correct rights.
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(hp, &rights));
  EXPECT_EQ(kDefaultDataPipeProducerHandleRights, rights);
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(hc, &rights));
  EXPECT_EQ(kDefaultDataPipeConsumerHandleRights, rights);

  // Shouldn't be able to duplicate either handle (just test "with reduced
  // rights" on one, and without on the other).
  MojoHandle handle_denied = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_PERMISSION_DENIED,
            MojoDuplicateHandleWithReducedRights(
                hp, MOJO_HANDLE_RIGHT_DUPLICATE, &handle_denied));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_denied);
  handle_denied = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_PERMISSION_DENIED,
            MojoDuplicateHandle(hc, &handle_denied));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_denied);

  // The consumer |hc| shouldn't be readable.
  MojoHandleSignalsState state;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_NONE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // The producer |hp| should be writable.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfiable_signals);

  // Try to read from |hc|.
  char buffer[20] = {};
  uint32_t buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_SHOULD_WAIT,
            MojoReadData(hc, buffer, &buffer_size, MOJO_READ_DATA_FLAG_NONE));

  // Try to begin a two-phase read from |hc|.
  const void* read_pointer = nullptr;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_SHOULD_WAIT,
            MojoBeginReadData(hc, &read_pointer, &buffer_size,
                              MOJO_READ_DATA_FLAG_NONE));

  // Write to |hp|.
  static const char kHello[] = "hello ";
  static const uint32_t kHelloLen = static_cast<uint32_t>(strlen(kHello));
  // Don't include terminating null.
  buffer_size = kHelloLen;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteData(hp, kHello, &buffer_size, MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kHelloLen, buffer_size);

  // |hc| should be(come) readable.
  MojoHandleSignals sig = MOJO_HANDLE_SIGNAL_READABLE;
  uint32_t result_index = 1;
  MojoHandleSignalsState states[1] = {};
  EXPECT_EQ(MOJO_RESULT_OK, MojoWaitMany(&hc, &sig, 1, MOJO_DEADLINE_INDEFINITE,
                                         &result_index, states));
  EXPECT_EQ(0u, result_index);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            states[0].satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            states[0].satisfiable_signals);

  // Do a two-phase write to |hp|.
  void* write_pointer = nullptr;
  ASSERT_EQ(MOJO_RESULT_OK, MojoBeginWriteData(hp, &write_pointer, &buffer_size,
                                               MOJO_WRITE_DATA_FLAG_NONE));
  static const char kWorld[] = "world";
  ASSERT_GE(buffer_size, sizeof(kWorld));
  // Include the terminating null.
  memcpy(write_pointer, kWorld, sizeof(kWorld));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoEndWriteData(hp, static_cast<uint32_t>(sizeof(kWorld))));

  // Read one character from |hc|.
  memset(buffer, 0, sizeof(buffer));
  buffer_size = 1;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, buffer, &buffer_size, MOJO_READ_DATA_FLAG_NONE));
  ASSERT_EQ(1u, buffer_size);
  EXPECT_EQ('h', buffer[0]);

  // Close |hp|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // |hc| should still be readable.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // Do a two-phase read from |hc|.
  read_pointer = nullptr;
  ASSERT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_pointer, &buffer_size,
                                              MOJO_READ_DATA_FLAG_NONE));
  ASSERT_LE(buffer_size, sizeof(buffer) - 1);
  memcpy(&buffer[1], read_pointer, buffer_size);
  EXPECT_EQ(MOJO_RESULT_OK, MojoEndReadData(hc, buffer_size));
  EXPECT_STREQ("hello world", buffer);

  // |hc| should no longer be readable.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, 1000, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  // TODO(vtl): Test the other way around -- closing the consumer should make
  // the producer never-writable?
}

TEST(DataPipeTest, WriteThreshold) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      2u,                                       // |element_num_bytes|.
      4u                                        // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  MojoDataPipeProducerOptions popts;
  static const uint32_t kPoptsSize = static_cast<uint32_t>(sizeof(popts));

  // Check the current write threshold; should be the default.
  memset(&popts, 255, kPoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeProducerOptions(hp, &popts, kPoptsSize));
  EXPECT_EQ(kPoptsSize, popts.struct_size);
  EXPECT_EQ(0u, popts.write_threshold_num_bytes);

  // Should already have the write threshold signal.
  MojoHandleSignalsState state = {};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfiable_signals);

  // Try setting the write threshold to something invalid.
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 1u;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoSetDataPipeProducerOptions(hp, &popts));
  // It shouldn't change the options.
  memset(&popts, 255, kPoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeProducerOptions(hp, &popts, kPoptsSize));
  EXPECT_EQ(kPoptsSize, popts.struct_size);
  EXPECT_EQ(0u, popts.write_threshold_num_bytes);

  // Write an element.
  static const uint16_t kTestElem = 12345u;
  uint32_t num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, &kTestElem, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(2u, num_bytes);

  // Should still have the write threshold signal.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0, nullptr));

  // Write another element.
  static const uint16_t kAnotherTestElem = 12345u;
  num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, &kAnotherTestElem, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(2u, num_bytes);

  // Should no longer have the write threshold signal.
  state = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0, &state));
  EXPECT_EQ(0u, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfiable_signals);

  // Set the write threshold to 2 (one element).
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeProducerOptions(hp, &popts));
  // It should actually change the options.
  memset(&popts, 255, kPoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeProducerOptions(hp, &popts, kPoptsSize));
  EXPECT_EQ(kPoptsSize, popts.struct_size);
  EXPECT_EQ(2u, popts.write_threshold_num_bytes);

  // Should still not have the write threshold signal.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0, nullptr));

  // Read an element.
  uint16_t read_elem = 0u;
  num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, &read_elem, &num_bytes, MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(2u, num_bytes);
  EXPECT_EQ(kTestElem, read_elem);

  // Should get the write threshold signal now.
  state = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 1000, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfiable_signals);

  // Set the write threshold to 4 (two elements).
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 4u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeProducerOptions(hp, &popts));

  // Should again not have the write threshold signal.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0, nullptr));

  // Close the consumer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  // The write threshold signal should now be unsatisfiable.
  state = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
}

TEST(DataPipeTest, ReadThreshold) {
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(nullptr, &hp, &hc);

  MojoDataPipeConsumerOptions copts;
  static const uint32_t kCoptsSize = static_cast<uint32_t>(sizeof(copts));

  // Check the current read threshold; should be the default.
  memset(&copts, 255, kCoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeConsumerOptions(hc, &copts, kCoptsSize));
  EXPECT_EQ(kCoptsSize, copts.struct_size);
  EXPECT_EQ(0u, copts.read_threshold_num_bytes);

  // Shouldn't have the read threshold signal yet.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, nullptr));

  // Write a byte to |hp|.
  static const char kAByte = 'A';
  uint32_t num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteData(hp, &kAByte, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);

  // Now should have the read threshold signal.
  MojoHandleSignalsState state;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // Set the read threshold to 3, and then check it.
  copts.struct_size = kCoptsSize;
  copts.read_threshold_num_bytes = 3u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeConsumerOptions(hc, &copts));

  memset(&copts, 255, kCoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeConsumerOptions(hc, &copts, kCoptsSize));
  EXPECT_EQ(kCoptsSize, copts.struct_size);
  EXPECT_EQ(3u, copts.read_threshold_num_bytes);

  // Shouldn't have the read threshold signal again.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0, nullptr));

  // Write another byte to |hp|.
  static const char kBByte = 'B';
  num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteData(hp, &kBByte, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);

  // Still shouldn't have the read threshold signal.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, nullptr));

  // Write a third byte to |hp|.
  static const char kCByte = 'C';
  num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteData(hp, &kCByte, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);

  // Now should have the read threshold signal.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, nullptr));

  // Read a byte.
  char read_byte = 'x';
  num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, &read_byte, &num_bytes, MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);
  EXPECT_EQ(kAByte, read_byte);

  // Shouldn't have the read threshold signal again.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0, nullptr));

  // Set the read threshold to 2.
  copts.struct_size = kCoptsSize;
  copts.read_threshold_num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeConsumerOptions(hc, &copts));

  // Should have the read threshold signal again.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0, nullptr));

  // Set the read threshold to the default by passing null, and check it.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeConsumerOptions(hc, nullptr));

  memset(&copts, 255, kCoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeConsumerOptions(hc, &copts, kCoptsSize));
  EXPECT_EQ(kCoptsSize, copts.struct_size);
  EXPECT_EQ(0u, copts.read_threshold_num_bytes);

  // Should still have the read threshold signal.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0, nullptr));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

TEST(DataPipeTest, Create) {
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  // Default options.
  Create(nullptr, &hp, &hc);
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  static constexpr MojoCreateDataPipeOptions kTestOptions[] = {
      // Trivial element size, non-default capacity.
      {kSizeOfOptions,                           // |struct_size|.
       MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
       1,                                        // |element_num_bytes|.
       1000},                                    // |capacity_num_bytes|.
      // Nontrivial element size, non-default capacity.
      {kSizeOfOptions,                           // |struct_size|.
       MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
       4,                                        // |element_num_bytes|.
       4000},                                    // |capacity_num_bytes|.
      // Nontrivial element size, default capacity.
      {kSizeOfOptions,                           // |struct_size|.
       MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
       100,                                      // |element_num_bytes|.
       0}                                        // |capacity_num_bytes|.
  };

  for (size_t i = 0; i < MOJO_ARRAYSIZE(kTestOptions); i++) {
    Create(&kTestOptions[i], &hp, &hc);
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
  }
}

TEST(DataPipeTest, SimpleReadWrite) {
  static constexpr MojoCreateDataPipeOptions options = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      1000 * sizeof(int32_t)                    // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&options, &hp, &hc);

  // Try reading; nothing there yet.
  int32_t elements[10] = {};
  uint32_t num_bytes =
      static_cast<uint32_t>(MOJO_ARRAYSIZE(elements) * sizeof(elements[0]));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_SHOULD_WAIT,
            MojoReadData(hc, elements, &num_bytes, MOJO_READ_DATA_FLAG_NONE));

  // Query; nothing there yet.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(0u, num_bytes);

  // Discard; nothing there yet.
  num_bytes = static_cast<uint32_t>(5u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_SHOULD_WAIT,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_DISCARD));

  // Read with invalid |num_bytes|.
  num_bytes = sizeof(elements[0]) + 1;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
            MojoReadData(hc, elements, &num_bytes, MOJO_READ_DATA_FLAG_NONE));

  // Check that the readable signal isn't set yet.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, nullptr));

  // Write two elements.
  elements[0] = 123;
  elements[1] = 456;
  num_bytes = static_cast<uint32_t>(2u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteData(hp, elements, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));
  // It should have written everything (even without "all or none").
  EXPECT_EQ(2u * sizeof(elements[0]), num_bytes);

  // Wait (should already be readable).
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kActionTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Query.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(2 * sizeof(elements[0]), num_bytes);

  // Read one element.
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, elements, &num_bytes, MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(1u * sizeof(elements[0]), num_bytes);
  EXPECT_EQ(123, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // Query.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(1 * sizeof(elements[0]), num_bytes);

  // Peek one element.
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, elements, &num_bytes, MOJO_READ_DATA_FLAG_PEEK));
  EXPECT_EQ(1u * sizeof(elements[0]), num_bytes);
  EXPECT_EQ(456, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // Query. Still has 1 element remaining.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(1 * sizeof(elements[0]), num_bytes);

  // Try to read two elements, with "all or none".
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(2u * sizeof(elements[0]));
  EXPECT_EQ(
      MOJO_RESULT_OUT_OF_RANGE,
      MojoReadData(hc, elements, &num_bytes, MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(-1, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // Try to read two elements, without "all or none".
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(2u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, elements, &num_bytes, MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(1u * sizeof(elements[0]), num_bytes);
  EXPECT_EQ(456, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // Query.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(0u, num_bytes);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// Note: The "basic" waiting tests test that the "wait states" are correct in
// various situations; they don't test that waiters are properly awoken on state
// changes. (For that, we need to use multiple threads.)
TEST(DataPipeTest, BasicProducerWaiting) {
  // Note: We take advantage of the fact that current for current
  // implementations capacities are strict maximums. This is not guaranteed by
  // the API.

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      2 * sizeof(int32_t)                       // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Never readable.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // Already writable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kZeroTimeout, &hss));

  // Write two elements.
  int32_t elements[2] = {123, 456};
  uint32_t num_bytes = static_cast<uint32_t>(2u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, elements, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(static_cast<uint32_t>(2u * sizeof(elements[0])), num_bytes);

  // It shouldn't be writable yet.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kZeroTimeout, &hss));
  EXPECT_EQ(0u, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // Wait for data to become available to the consumer.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Peek one element.
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, elements, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE |
                                             MOJO_READ_DATA_FLAG_PEEK));
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);
  EXPECT_EQ(123, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // It still shouldn't be writable yet.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kZeroTimeout, &hss));
  EXPECT_EQ(0u, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // Do it again; read one element.
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, elements, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);
  EXPECT_EQ(123, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // Waiting should now succeed.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // Try writing, using a two-phase write.
  void* buffer = nullptr;
  num_bytes = static_cast<uint32_t>(3u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginWriteData(hp, &buffer, &num_bytes,
                                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_TRUE(buffer);
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);

  static_cast<int32_t*>(buffer)[0] = 789;
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoEndWriteData(hp, static_cast<uint32_t>(1u * sizeof(elements[0]))));

  // Read one element, using a two-phase read.
  const void* read_buffer = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_buffer, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_TRUE(read_buffer);
  // Since we only read one element (after having written three in all), the
  // two-phase read should only allow us to read one. This checks an
  // implementation detail!
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);
  EXPECT_EQ(456, static_cast<const int32_t*>(read_buffer)[0]);
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoEndReadData(hc, static_cast<uint32_t>(1u * sizeof(elements[0]))));

  // Waiting should succeed.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // Write one element.
  elements[0] = 123;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteData(hp, elements, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);

  // Close the consumer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  // It should now be never-writable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
}

TEST(DataPipeTest, PeerClosedProducerWaiting) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      2 * sizeof(int32_t)                       // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Close the consumer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  // "Peer closed" should be signaled on the producer.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_PEER_CLOSED, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
}

TEST(DataPipeTest, PeerClosedConsumerWaiting) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      2 * sizeof(int32_t)                       // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // "Peer closed" should be signaled on the consumer.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_PEER_CLOSED, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

TEST(DataPipeTest, BasicConsumerWaiting) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      1000 * sizeof(int32_t)                    // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Never writable.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_WRITABLE, kZeroTimeout, &hss));
  EXPECT_EQ(0u, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Write two elements.
  int32_t elements[2] = {123, 456};
  uint32_t num_bytes = static_cast<uint32_t>(2u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, &elements, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));

  // Wait for readability.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Discard one element.
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, nullptr, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE |
                                             MOJO_READ_DATA_FLAG_DISCARD));
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);

  // Should still be readable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Peek one element.
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, elements, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE |
                                             MOJO_READ_DATA_FLAG_PEEK));
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);
  EXPECT_EQ(456, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // Should still be readable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Read one element.
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, elements, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);
  EXPECT_EQ(456, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // Write one element.
  elements[0] = 789;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, elements, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));

  // Waiting should succeed.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // Should still be readable (even if the peer closed signal hasn't propagated
  // yet).
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  // We don't know if the peer closed signal has propagated yet.
  EXPECT_TRUE((hss.satisfied_signals & MOJO_HANDLE_SIGNAL_READABLE));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Wait for the peer closed signal.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_PEER_CLOSED, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Read one element.
  elements[0] = -1;
  elements[1] = -1;
  num_bytes = static_cast<uint32_t>(1u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, elements, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);
  EXPECT_EQ(789, elements[0]);
  EXPECT_EQ(-1, elements[1]);

  // Should be never-readable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// Test with two-phase APIs and also closing the producer with an active
// consumer waiter.
TEST(DataPipeTest, ConsumerWaitingTwoPhase) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      1000 * sizeof(int32_t)                    // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Write two elements.
  int32_t* elements = nullptr;
  void* buffer = nullptr;
  uint32_t num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginWriteData(hp, &buffer, &num_bytes,
                                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_TRUE(buffer);
  EXPECT_GE(num_bytes, static_cast<uint32_t>(3u * sizeof(elements[0])));
  elements = static_cast<int32_t*>(buffer);
  elements[0] = 123;
  elements[1] = 456;
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoEndWriteData(hp, static_cast<uint32_t>(2u * sizeof(elements[0]))));

  // Wait for readability.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Read one element.
  const void* read_buffer = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_buffer, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_TRUE(read_buffer);
  EXPECT_EQ(static_cast<uint32_t>(2u * sizeof(elements[0])), num_bytes);
  const int32_t* read_elements = static_cast<const int32_t*>(read_buffer);
  EXPECT_EQ(123, read_elements[0]);
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoEndReadData(hc, static_cast<uint32_t>(1u * sizeof(elements[0]))));

  // Should still be readable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Read one element.
  // Request three, but not in all-or-none mode.
  read_buffer = nullptr;
  num_bytes = static_cast<uint32_t>(3u * sizeof(elements[0]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_buffer, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_TRUE(read_buffer);
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(elements[0])), num_bytes);
  read_elements = static_cast<const int32_t*>(read_buffer);
  EXPECT_EQ(456, read_elements[0]);
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoEndReadData(hc, static_cast<uint32_t>(1u * sizeof(elements[0]))));

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // Should be never-readable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// Tests that data pipes aren't writable/readable during two-phase writes/reads.
TEST(DataPipeTest, BasicTwoPhaseWaiting) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      1000 * sizeof(int32_t)                    // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // It should be writable.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  void* write_ptr = nullptr;
  uint32_t num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginWriteData(hp, &write_ptr, &num_bytes,
                                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_TRUE(write_ptr);
  EXPECT_GE(num_bytes, static_cast<uint32_t>(1u * sizeof(int32_t)));

  // At this point, it shouldn't be writable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kZeroTimeout, &hss));
  EXPECT_EQ(0u, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  static_cast<int32_t*>(write_ptr)[0] = 123;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoEndWriteData(hp, static_cast<uint32_t>(1u * sizeof(int32_t))));

  // It should immediately be writable again.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // It should become readable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Start another two-phase write and check that it's readable even in the
  // middle of it.
  write_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginWriteData(hp, &write_ptr, &num_bytes,
                                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_TRUE(write_ptr);
  EXPECT_GE(num_bytes, static_cast<uint32_t>(1u * sizeof(int32_t)));

  // It should be readable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // End the two-phase write without writing anything.
  EXPECT_EQ(MOJO_RESULT_OK, MojoEndWriteData(hp, 0u));

  // Start a two-phase read.
  const void* read_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_ptr, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_TRUE(read_ptr);
  EXPECT_EQ(static_cast<uint32_t>(1u * sizeof(int32_t)), num_bytes);

  // At this point, it should still be writable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // But not readable.
  hss = MojoHandleSignalsState{0u, 0u};
  ASSERT_EQ(MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  EXPECT_EQ(0u, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // End the two-phase read without reading anything.
  EXPECT_EQ(MOJO_RESULT_OK, MojoEndReadData(hc, 0u));

  // It should be readable again.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

void Seq(int32_t start, size_t count, int32_t* out) {
  for (size_t i = 0; i < count; i++)
    out[i] = start + static_cast<int32_t>(i);
}

TEST(DataPipeTest, AllOrNone) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      10 * sizeof(int32_t)                      // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Try writing way too much.
  uint32_t num_bytes = 20u * sizeof(int32_t);
  int32_t buffer[100];
  Seq(0, MOJO_ARRAYSIZE(buffer), buffer);
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_OUT_OF_RANGE,
      MojoWriteData(hp, buffer, &num_bytes, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));

  // Should still be empty.
  num_bytes = ~0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(0u, num_bytes);

  // Write some data.
  num_bytes = 5u * sizeof(int32_t);
  Seq(100, MOJO_ARRAYSIZE(buffer), buffer);
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, buffer, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(5u * sizeof(int32_t), num_bytes);

  // Wait for data.
  // TODO(vtl): There's no real guarantee that all the data will become
  // available at once (except that in current implementations, with reasonable
  // limits, it will). Eventually, we'll be able to wait for a specified amount
  // of data to become available.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Half full.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(5u * sizeof(int32_t), num_bytes);

  // Too much.
  num_bytes = 6u * sizeof(int32_t);
  Seq(200, MOJO_ARRAYSIZE(buffer), buffer);
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_OUT_OF_RANGE,
      MojoWriteData(hp, buffer, &num_bytes, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));

  // Try reading too much.
  num_bytes = 11u * sizeof(int32_t);
  memset(buffer, 0xab, sizeof(buffer));
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_OUT_OF_RANGE,
      MojoReadData(hc, buffer, &num_bytes, MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  int32_t expected_buffer[100];
  memset(expected_buffer, 0xab, sizeof(expected_buffer));
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, sizeof(buffer)));

  // Try discarding too much.
  num_bytes = 11u * sizeof(int32_t);
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_OUT_OF_RANGE,
      MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_ALL_OR_NONE |
                                                MOJO_READ_DATA_FLAG_DISCARD));

  // Just a little.
  num_bytes = 2u * sizeof(int32_t);
  Seq(300, MOJO_ARRAYSIZE(buffer), buffer);
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, buffer, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(2u * sizeof(int32_t), num_bytes);

  // Just right.
  num_bytes = 3u * sizeof(int32_t);
  Seq(400, MOJO_ARRAYSIZE(buffer), buffer);
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, buffer, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(3u * sizeof(int32_t), num_bytes);

  WaitForData(hc, static_cast<uint32_t>(10u * sizeof(int32_t)), kTinyTimeout);
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(10u * sizeof(int32_t), num_bytes);

  // Read half.
  num_bytes = 5u * sizeof(int32_t);
  memset(buffer, 0xab, sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, buffer, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(5u * sizeof(int32_t), num_bytes);
  memset(expected_buffer, 0xab, sizeof(expected_buffer));
  Seq(100, 5, expected_buffer);
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, sizeof(buffer)));

  // Try reading too much again.
  num_bytes = 6u * sizeof(int32_t);
  memset(buffer, 0xab, sizeof(buffer));
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_OUT_OF_RANGE,
      MojoReadData(hc, buffer, &num_bytes, MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  memset(expected_buffer, 0xab, sizeof(expected_buffer));
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, sizeof(buffer)));

  // Try discarding too much again.
  num_bytes = 6u * sizeof(int32_t);
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_OUT_OF_RANGE,
      MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_ALL_OR_NONE |
                                                MOJO_READ_DATA_FLAG_DISCARD));

  // Discard a little.
  num_bytes = 2u * sizeof(int32_t);
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, nullptr, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE |
                                             MOJO_READ_DATA_FLAG_DISCARD));
  EXPECT_EQ(2u * sizeof(int32_t), num_bytes);

  // Three left.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(3u * sizeof(int32_t), num_bytes);

  // Close the producer, then test producer-closed cases.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // Wait.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_PEER_CLOSED, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Try reading too much; "failed precondition" since the producer is closed.
  num_bytes = 4u * sizeof(int32_t);
  memset(buffer, 0xab, sizeof(buffer));
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
      MojoReadData(hc, buffer, &num_bytes, MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  memset(expected_buffer, 0xab, sizeof(expected_buffer));
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, sizeof(buffer)));

  // Try discarding too much; "failed precondition" again.
  num_bytes = 4u * sizeof(int32_t);
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
      MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_ALL_OR_NONE |
                                                MOJO_READ_DATA_FLAG_DISCARD));

  // Read a little.
  num_bytes = 2u * sizeof(int32_t);
  memset(buffer, 0xab, sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, buffer, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(2u * sizeof(int32_t), num_bytes);
  memset(expected_buffer, 0xab, sizeof(expected_buffer));
  Seq(400, 2, expected_buffer);
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, sizeof(buffer)));

  // Discard the remaining element.
  num_bytes = 1u * sizeof(int32_t);
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, nullptr, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE |
                                             MOJO_READ_DATA_FLAG_DISCARD));
  EXPECT_EQ(1u * sizeof(int32_t), num_bytes);

  // Empty again.
  num_bytes = ~0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(0u, num_bytes);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// Tests that |ProducerWriteData()| and |ConsumerReadData()| writes and reads,
// respectively, as much as possible, even if it may have to "wrap around" the
// internal circular buffer. (Note that the two-phase write and read need not do
// this.)
TEST(DataPipeTest, WrapAround) {
  unsigned char test_data[1000];
  for (size_t i = 0; i < MOJO_ARRAYSIZE(test_data); i++)
    test_data[i] = static_cast<unsigned char>(i);

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      100u                                      // |capacity_num_bytes|.
  };
  // TODO(vtl): This test won't be valid if more space is actually provided in
  // the pipe than requested.
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Write 20 bytes.
  uint32_t num_bytes = 20u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, &test_data[0], &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(20u, num_bytes);

  // Wait for data.
  // TODO(vtl): (See corresponding TODO in AllOrNone.)
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Read 10 bytes.
  unsigned char read_buffer[1000] = {0};
  num_bytes = 10u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, read_buffer, &num_bytes,
                                         MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(10u, num_bytes);
  EXPECT_EQ(0, memcmp(read_buffer, &test_data[0], 10u));

  num_bytes = 90u;
  WaitForSpace(hp, num_bytes, kTinyTimeout);
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, &test_data[20], &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(90u, num_bytes);

  WaitForData(hc, 100u, kTinyTimeout);
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(100u, num_bytes);

  // Read as much as possible (using |ConsumerReadData()|). We should read 100
  // bytes.
  num_bytes = static_cast<uint32_t>(sizeof(read_buffer));
  memset(read_buffer, 0, num_bytes);
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, read_buffer, &num_bytes,
                                         MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(100u, num_bytes);
  EXPECT_EQ(0, memcmp(read_buffer, &test_data[10], 100u));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// Tests the behavior of writing (simple and two-phase), closing the producer,
// then reading (simple and two-phase).
TEST(DataPipeTest, WriteCloseProducerRead) {
  const char kTestData[] = "hello world";
  const uint32_t kTestDataSize = static_cast<uint32_t>(sizeof(kTestData));

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      1000u                                     // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Write some data, so we'll have something to read.
  uint32_t num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Write it again, so we'll have something left over.
  num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Start two-phase write.
  void* write_buffer_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoBeginWriteData(hp, &write_buffer_ptr, &num_bytes,
                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_TRUE(write_buffer_ptr);
  EXPECT_GT(num_bytes, 0u);

  WaitForData(hc, 2u * kTestDataSize, kTinyTimeout);
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(2u * kTestDataSize, num_bytes);

  // Start two-phase read.
  const void* read_buffer_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_buffer_ptr, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_TRUE(read_buffer_ptr);
  EXPECT_EQ(2u * kTestDataSize, num_bytes);

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // The consumer can finish its two-phase read.
  EXPECT_EQ(0, memcmp(read_buffer_ptr, kTestData, kTestDataSize));
  EXPECT_EQ(MOJO_RESULT_OK, MojoEndReadData(hc, kTestDataSize));

  // And start another.
  read_buffer_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_buffer_ptr, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_TRUE(read_buffer_ptr);
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Close the consumer, which cancels the two-phase read.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// Tests the behavior of interrupting a two-phase read and write by closing the
// consumer.
TEST(DataPipeTest, TwoPhaseWriteReadCloseConsumer) {
  const char kTestData[] = "hello world";
  const uint32_t kTestDataSize = static_cast<uint32_t>(sizeof(kTestData));

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      1000u                                     // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Write some data, so we'll have something to read.
  uint32_t num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Start two-phase write.
  void* write_buffer_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoBeginWriteData(hp, &write_buffer_ptr, &num_bytes,
                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_TRUE(write_buffer_ptr);
  ASSERT_GT(num_bytes, kTestDataSize);

  // Wait for data.
  WaitForData(hc, kTestDataSize, kTinyTimeout);

  // Start two-phase read.
  const void* read_buffer_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_buffer_ptr, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_TRUE(read_buffer_ptr);
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Close the consumer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  // Wait for producer to know that the consumer is closed.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_PEER_CLOSED, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  // Actually write some data. (Note: Premature freeing of the buffer would
  // probably only be detected under ASAN or similar.)
  memcpy(write_buffer_ptr, kTestData, kTestDataSize);
  // Note: Even though the consumer has been closed, ending the two-phase
  // write will report success.
  EXPECT_EQ(MOJO_RESULT_OK, MojoEndWriteData(hp, kTestDataSize));

  // But trying to write should result in failure.
  num_bytes = kTestDataSize;
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
      MojoWriteData(hp, kTestData, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));

  // As will trying to start another two-phase write.
  write_buffer_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoBeginWriteData(hp, &write_buffer_ptr, &num_bytes,
                               MOJO_WRITE_DATA_FLAG_NONE));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
}

// Tests the behavior of "interrupting" a two-phase write by closing both the
// producer and the consumer.
TEST(DataPipeTest, TwoPhaseWriteCloseBoth) {
  const uint32_t kTestDataSize = 15u;

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      1000u                                     // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Start two-phase write.
  void* write_buffer_ptr = nullptr;
  uint32_t num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoBeginWriteData(hp, &write_buffer_ptr, &num_bytes,
                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_TRUE(write_buffer_ptr);
  ASSERT_GT(num_bytes, kTestDataSize);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
}

// Tests the behavior of writing, closing the producer, and then reading (with
// and without data remaining).
TEST(DataPipeTest, WriteCloseProducerReadNoData) {
  const char kTestData[] = "hello world";
  const uint32_t kTestDataSize = static_cast<uint32_t>(sizeof(kTestData));

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      1000u                                     // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Write some data, so we'll have something to read.
  uint32_t num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // Wait. (Note that once the consumer knows that the producer is closed, it
  // must also know about all the data that was sent.)
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_PEER_CLOSED, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Peek that data.
  char buffer[1000];
  num_bytes = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, buffer, &num_bytes, MOJO_READ_DATA_FLAG_PEEK));
  EXPECT_EQ(kTestDataSize, num_bytes);
  EXPECT_EQ(0, memcmp(buffer, kTestData, kTestDataSize));

  // Read that data.
  memset(buffer, 0, 1000);
  num_bytes = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, buffer, &num_bytes, MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);
  EXPECT_EQ(0, memcmp(buffer, kTestData, kTestDataSize));

  // A second read should fail.
  num_bytes = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoReadData(hc, buffer, &num_bytes, MOJO_READ_DATA_FLAG_NONE));

  // A two-phase read should also fail.
  const void* read_buffer_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoBeginReadData(hc, &read_buffer_ptr, &num_bytes,
                              MOJO_READ_DATA_FLAG_NONE));

  // Ditto for discard.
  num_bytes = 10u;
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_DISCARD));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// Tests the behavior of writing, reading (all the data), closing the producer,
// and then waiting for more data (with no data remaining).
TEST(DataPipeTest, WriteReadCloseProducerWaitNoData) {
  const int64_t kTestData = 123456789012345LL;
  const uint32_t kTestDataSize = static_cast<uint32_t>(sizeof(kTestData));

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      kTestDataSize,                            // |element_num_bytes|.
      100u * kTestDataSize                      // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Write some data, so we'll have something to read.
  uint32_t num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, &kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Wait.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Read that data.
  int64_t data[10] = {};
  num_bytes = static_cast<uint32_t>(sizeof(data));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, data, &num_bytes, MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);
  EXPECT_EQ(kTestData, data[0]);

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // Wait.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// During a two-phase read, the consumer is not readable so it may be waited
// upon (to become readable again). If the producer is closed and the two-phase
// read consumes the remaining data, that wait should become unsatisfiable.
TEST(DataPipeTest, BeginReadCloseProducerWaitEndReadNoData) {
  const int64_t kTestData = 123456789012345LL;
  const uint32_t kTestDataSize = static_cast<uint32_t>(sizeof(kTestData));

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      kTestDataSize,                            // |element_num_bytes|.
      100u * kTestDataSize                      // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Write some data, so we'll have something to read.
  uint32_t num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, &kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Wait.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Start a two-phase read.
  num_bytes = 0u;
  const void* read_ptr = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_ptr, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);
  EXPECT_EQ(kTestData, static_cast<const int64_t*>(read_ptr)[0]);

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // Wait for producer close to be detected.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_PEER_CLOSED, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Complete the two-phase read.
  EXPECT_EQ(MOJO_RESULT_OK, MojoEndReadData(hc, kTestDataSize));

  // Wait.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// During a two-phase write, the producer is not writable so it may be waited
// upon (to become writable again). If the consumer is closed, that wait should
// become unsatisfiable.
TEST(DataPipeTest, BeginWriteCloseConsumerWaitEndWrite) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      100u                                      // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Start a two-phase write.
  void* write_ptr = nullptr;
  uint32_t num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginWriteData(hp, &write_ptr, &num_bytes,
                                               MOJO_WRITE_DATA_FLAG_NONE));

  // Close the consumer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  // Wait for the consumer close to be detected.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_PEER_CLOSED, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  // Complete the two-phase write (with nothing written).
  EXPECT_EQ(MOJO_RESULT_OK, MojoEndWriteData(hp, 0u));

  // Wait.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
}

// Test that two-phase reads/writes behave correctly when given invalid
// arguments.
TEST(DataPipeTest, TwoPhaseMoreInvalidArguments) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      static_cast<uint32_t>(sizeof(int32_t)),   // |element_num_bytes|.
      10 * sizeof(int32_t)                      // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // No data.
  uint32_t num_bytes = 1000u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(0u, num_bytes);

  // Try "ending" a two-phase write when one isn't active.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoEndWriteData(hp, 1u * sizeof(int32_t)));

  // Still no data.
  num_bytes = 1000u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(0u, num_bytes);

  // Try ending a two-phase write with an invalid amount (too much).
  void* write_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginWriteData(hp, &write_ptr, &num_bytes,
                                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
      MojoEndWriteData(hp, num_bytes + static_cast<uint32_t>(sizeof(int32_t))));

  // But the two-phase write still ended.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION, MojoEndWriteData(hp, 0u));

  // Still no data.
  num_bytes = 1000u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(0u, num_bytes);

  // Try ending a two-phase write with an invalid amount (not a multiple of the
  // element size).
  write_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginWriteData(hp, &write_ptr, &num_bytes,
                                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_GE(num_bytes, 1u);
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT, MojoEndWriteData(hp, 1u));

  // But the two-phase write still ended.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION, MojoEndWriteData(hp, 0u));

  // Still no data.
  num_bytes = 1000u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(0u, num_bytes);

  // Now write some data, so we'll be able to try reading.
  int32_t element = 123;
  num_bytes = 1u * sizeof(int32_t);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteData(hp, &element, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));

  // Wait for data.
  // TODO(vtl): (See corresponding TODO in AllOrNone.)
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // One element available.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(1u * sizeof(int32_t), num_bytes);

  // Try "ending" a two-phase read when one isn't active.
  EXPECT_EQ(MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
            MojoEndReadData(hc, 1u * sizeof(int32_t)));

  // Still one element available.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(1u * sizeof(int32_t), num_bytes);

  // Try ending a two-phase read with an invalid amount (too much).
  num_bytes = 0u;
  const void* read_ptr = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_ptr, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_INVALID_ARGUMENT,
      MojoEndReadData(hc, num_bytes + static_cast<uint32_t>(sizeof(int32_t))));

  // Still one element available.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(1u * sizeof(int32_t), num_bytes);

  // Try ending a two-phase read with an invalid amount (not a multiple of the
  // element size).
  num_bytes = 0u;
  read_ptr = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_ptr, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(1u * sizeof(int32_t), num_bytes);
  EXPECT_EQ(123, static_cast<const int32_t*>(read_ptr)[0]);
  EXPECT_EQ(MOJO_SYSTEM_RESULT_INVALID_ARGUMENT, MojoEndReadData(hc, 1u));

  // Still one element available.
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(1u * sizeof(int32_t), num_bytes);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// Tests the behavior of writing, closing the producer, and then doing a
// two-phase read of all the data.
// Note: If this test fails/crashes flakily, this is almost certainly an
// indication of a problem in the implementation and not the test.
TEST(DataPipeTest, WriteCloseProducerTwoPhaseReadAllData) {
  const char kTestData[] = "hello world";
  const uint32_t kTestDataSize = static_cast<uint32_t>(sizeof(kTestData));

  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      1000u                                     // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  // Write some data, so we'll have something to read.
  uint32_t num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  WaitForData(hc, kTestDataSize, kTinyTimeout);
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, nullptr, &num_bytes, MOJO_READ_DATA_FLAG_QUERY));
  EXPECT_EQ(kTestDataSize, num_bytes);

  const void* read_buffer_ptr = nullptr;
  num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoBeginReadData(hc, &read_buffer_ptr, &num_bytes,
                                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);
  EXPECT_EQ(0, memcmp(read_buffer_ptr, kTestData, kTestDataSize));

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  EXPECT_EQ(MOJO_RESULT_OK, MojoEndReadData(hc, num_bytes));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// TODO(vtl): This and DataPipeTest.WriteThreshold have nontrivial overlap (I've
// already deleted some of the overlap). Possibly this should just be deleted.
TEST(DataPipeTest, WriteThreshold2) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      10u                                       // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  MojoDataPipeProducerOptions popts;
  static const uint32_t kPoptsSize = static_cast<uint32_t>(sizeof(popts));

  // Try to wait to the write threshold signal; it should already have it.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
                                     kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // Write 5 bytes.
  static const char kTestData[5] = {'A', 'B', 'C', 'D', 'E'};
  uint32_t num_bytes = 5;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(5u, num_bytes);

  // It should still have the write threshold signal.
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
                                     kZeroTimeout, nullptr));

  // Set the write threshold to 5.
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 5u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeProducerOptions(hp, &popts));
  popts.write_threshold_num_bytes = 123u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeProducerOptions(hp, &popts, kPoptsSize));
  EXPECT_EQ(5u, popts.write_threshold_num_bytes);

  // Should still have the write threshold signal.
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
                                     kZeroTimeout, nullptr));

  // Set the write threshold to 6.
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 6u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeProducerOptions(hp, &popts));

  // Should no longer have the write threshold signal.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
      MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // Wait for the consumer to be readable.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(hc, MOJO_HANDLE_SIGNAL_READABLE, kTinyTimeout, nullptr));

  // Read a byte.
  char read_byte = 'a';
  num_bytes = sizeof(read_byte);
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, &read_byte, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(1u, num_bytes);
  EXPECT_EQ(kTestData[0], read_byte);

  // Wait; should get the write threshold signal.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
                                     kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            hss.satisfiable_signals);

  // Write 6 more bytes.
  static const char kMoreTestData[6] = {'1', '2', '3', '4', '5', '6'};
  num_bytes = 6;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kMoreTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(6u, num_bytes);

  // Should no longer have the write threshold signal.
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
      MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, kZeroTimeout, nullptr));

  // Set the write threshold to 0 (which means the element size, 1).
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeProducerOptions(hp, &popts));
  popts.write_threshold_num_bytes = 123u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeProducerOptions(hp, &popts, kPoptsSize));
  EXPECT_EQ(0u, popts.write_threshold_num_bytes);

  // Should still not have the write threshold signal.
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
      MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, kZeroTimeout, nullptr));

  // Close the consumer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  // Wait; the condition should now never be satisfiable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
      MojoWait(hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));
}

// TODO(vtl): This and DataPipeTest.ReadThreshold have nontrivial overlap (I've
// already deleted some of the overlap). Possibly this should just be deleted.
TEST(DataPipeTest, ReadThreshold2) {
  static constexpr MojoCreateDataPipeOptions kOptions = {
      kSizeOfOptions,                           // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      1u,                                       // |element_num_bytes|.
      1000u                                     // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  Create(&kOptions, &hp, &hc);

  MojoDataPipeConsumerOptions copts;
  static const uint32_t kCoptsSize = static_cast<uint32_t>(sizeof(copts));

  // Trivial wait: it shouldn't have the read threshold signal.
  MojoHandleSignalsState hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
      MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, kZeroTimeout, &hss));
  EXPECT_EQ(0u, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Write a byte.
  const char kTestData[] = {'x'};
  const uint32_t kTestDataSize = static_cast<uint32_t>(sizeof(kTestData));
  uint32_t num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Wait for the read threshold signal.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
                                     kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Set the read threshold to 1.
  copts.struct_size = kCoptsSize;
  copts.read_threshold_num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeConsumerOptions(hc, &copts));
  copts.read_threshold_num_bytes = 123u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeConsumerOptions(hc, &copts, kCoptsSize));
  EXPECT_EQ(1u, copts.read_threshold_num_bytes);

  // Wait: it should (still) already have the read threshold signal.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
                                     kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Set the read threshold to 2.
  copts.struct_size = kCoptsSize;
  copts.read_threshold_num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeConsumerOptions(hc, &copts));
  copts.read_threshold_num_bytes = 123u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeConsumerOptions(hc, &copts, kCoptsSize));
  EXPECT_EQ(2u, copts.read_threshold_num_bytes);

  // Write another byte.
  num_bytes = kTestDataSize;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteData(hp, kTestData, &num_bytes,
                                          MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(kTestDataSize, num_bytes);

  // Wait for the read threshold signal.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
                                     kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Read one byte.
  char read_byte = 'a';
  num_bytes = sizeof(read_byte);
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, &read_byte, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(1u, num_bytes);
  EXPECT_EQ(kTestData[0], read_byte);

  // Trivial wait: it shouldn't have the read threshold signal.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_DEADLINE_EXCEEDED,
      MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Close the producer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // Wait; the current read threshold becomes never satisfiable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
      MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            hss.satisfiable_signals);

  // Set the read threshold back to zero to 0.
  copts.struct_size = kCoptsSize;
  copts.read_threshold_num_bytes = 0u;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSetDataPipeConsumerOptions(hc, &copts));
  copts.read_threshold_num_bytes = 123u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoGetDataPipeConsumerOptions(hc, &copts, kCoptsSize));
  // "Get options" should preserve 0 (and not set it to the element size).
  EXPECT_EQ(0u, copts.read_threshold_num_bytes);

  // Wait: it should have the read threshold signal.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
                                     kTinyTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            hss.satisfiable_signals);

  // Read the other byte.
  read_byte = 'a';
  num_bytes = sizeof(read_byte);
  EXPECT_EQ(MOJO_RESULT_OK, MojoReadData(hc, &read_byte, &num_bytes,
                                         MOJO_READ_DATA_FLAG_ALL_OR_NONE));
  EXPECT_EQ(1u, num_bytes);
  EXPECT_EQ(kTestData[0], read_byte);

  // Wait: the read threshold signal should be unsatisfiable.
  hss = MojoHandleSignalsState{0u, 0u};
  EXPECT_EQ(
      MOJO_SYSTEM_RESULT_FAILED_PRECONDITION,
      MojoWait(hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, kZeroTimeout, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));
}

// TODO(vtl): Port options validation tests from the EDK's
// data_pipe_unittest.cc.

// TODO(vtl): Add multi-threaded tests.

}  // namespace
