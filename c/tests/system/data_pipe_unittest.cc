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
  state = MojoHandleSignalsState();
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
  state = MojoHandleSignalsState();
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
  state = MojoHandleSignalsState();
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

// TODO(vtl): Port more tests from the EDK's data_pipe_impl_unittest.cc.

// TODO(vtl): Add multi-threaded tests.

}  // namespace
