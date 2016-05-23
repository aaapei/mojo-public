// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C API, but using the explicit MojoSystemImpl parameter.

#include <string.h>

#include "mojo/public/platform/native/system_impl_private.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

const MojoHandleRights kDefaultMessagePipeHandleRights =
    MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_READ |
    MOJO_HANDLE_RIGHT_WRITE | MOJO_HANDLE_RIGHT_GET_OPTIONS |
    MOJO_HANDLE_RIGHT_SET_OPTIONS;
const MojoHandleRights kDefaultDataPipeProducerHandleRights =
    MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_WRITE |
    MOJO_HANDLE_RIGHT_GET_OPTIONS | MOJO_HANDLE_RIGHT_SET_OPTIONS;
const MojoHandleRights kDefaultDataPipeConsumerHandleRights =
    MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_READ |
    MOJO_HANDLE_RIGHT_GET_OPTIONS | MOJO_HANDLE_RIGHT_SET_OPTIONS;
const MojoHandleRights kDefaultSharedBufferHandleRights =
    MOJO_HANDLE_RIGHT_DUPLICATE | MOJO_HANDLE_RIGHT_TRANSFER |
    MOJO_HANDLE_RIGHT_GET_OPTIONS | MOJO_HANDLE_RIGHT_SET_OPTIONS |
    MOJO_HANDLE_RIGHT_MAP_READABLE | MOJO_HANDLE_RIGHT_MAP_WRITABLE |
    MOJO_HANDLE_RIGHT_MAP_EXECUTABLE;

TEST(SystemImplTest, GetTimeTicksNow) {
  MojoSystemImpl system = MojoSystemImplCreateImpl();
  const MojoTimeTicks start = MojoSystemImplGetTimeTicksNow(system);
  EXPECT_NE(static_cast<MojoTimeTicks>(0), start)
      << "MojoGetTimeTicksNow should return nonzero value";

  // SystemImpl is leaked...
}

TEST(SystemImplTest, BasicMessagePipe) {
  MojoSystemImpl sys0 = MojoSystemImplCreateImpl();
  MojoSystemImpl sys1 = MojoSystemImplCreateImpl();
  EXPECT_NE(sys0, sys1);

  MojoHandle h0, h1;
  MojoHandleSignals sig;
  char buffer[10] = {0};
  uint32_t buffer_size;

  h0 = MOJO_HANDLE_INVALID;
  h1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplCreateMessagePipe(sys0, nullptr, &h0, &h1));
  EXPECT_NE(h0, MOJO_HANDLE_INVALID);
  EXPECT_NE(h1, MOJO_HANDLE_INVALID);
  EXPECT_NE(h0, h1);

  // Both handles should have the correct rights.
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetRights(sys0, h0, &rights));
  EXPECT_EQ(kDefaultMessagePipeHandleRights, rights);
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetRights(sys0, h1, &rights));
  EXPECT_EQ(kDefaultMessagePipeHandleRights, rights);

  // Shouldn't be able to duplicate either handle (just test "with reduced
  // rights" on one, and without on the other).
  MojoHandle handle_denied = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED,
            MojoSystemImplDuplicateHandleWithReducedRights(
                sys0, h0, MOJO_HANDLE_RIGHT_DUPLICATE, &handle_denied));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_denied);
  handle_denied = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED,
            MojoSystemImplDuplicateHandle(sys0, h1, &handle_denied));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_denied);

  // Move the other end of the pipe to a different SystemImpl.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplTransferHandle(sys0, h1, sys1, &h1));
  EXPECT_NE(h1, MOJO_HANDLE_INVALID);

  // Shouldn't be readable, we haven't written anything.
  MojoHandleSignalsState state;
  EXPECT_EQ(
      MOJO_RESULT_DEADLINE_EXCEEDED,
      MojoSystemImplWait(sys0, h0, MOJO_HANDLE_SIGNAL_READABLE, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            state.satisfiable_signals);

  // Should be writable.
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoSystemImplWait(sys0, h0, MOJO_HANDLE_SIGNAL_WRITABLE, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            state.satisfiable_signals);

  // Last parameter is optional.
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoSystemImplWait(sys0, h0, MOJO_HANDLE_SIGNAL_WRITABLE, 0, nullptr));

  // Try to read.
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_SHOULD_WAIT,
            MojoSystemImplReadMessage(sys0, h0, buffer, &buffer_size, nullptr,
                                      nullptr, MOJO_READ_MESSAGE_FLAG_NONE));

  // Write to |h1|.
  static const char kHello[] = "hello";
  buffer_size = static_cast<uint32_t>(sizeof(kHello));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWriteMessage(sys1, h1, kHello, buffer_size, nullptr,
                                       0, MOJO_WRITE_MESSAGE_FLAG_NONE));

  // |h0| should be readable.
  uint32_t result_index = 1;
  MojoHandleSignalsState states[1];
  sig = MOJO_HANDLE_SIGNAL_READABLE;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWaitMany(sys0, &h0, &sig, 1, MOJO_DEADLINE_INDEFINITE,
                                   &result_index, states));

  EXPECT_EQ(0u, result_index);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
            states[0].satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            states[0].satisfiable_signals);

  // Read from |h0|.
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplReadMessage(sys0, h0, buffer, &buffer_size, nullptr,
                                      nullptr, MOJO_READ_MESSAGE_FLAG_NONE));
  EXPECT_EQ(static_cast<uint32_t>(sizeof(kHello)), buffer_size);
  EXPECT_STREQ(kHello, buffer);

  // |h0| should no longer be readable.
  EXPECT_EQ(
      MOJO_RESULT_DEADLINE_EXCEEDED,
      MojoSystemImplWait(sys0, h0, MOJO_HANDLE_SIGNAL_READABLE, 10, &state));

  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            state.satisfiable_signals);

  // Close |h0|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys0, h0));

  // |h1| should no longer be readable or writable.
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoSystemImplWait(sys1, h1, MOJO_HANDLE_SIGNAL_READABLE |
                                             MOJO_HANDLE_SIGNAL_WRITABLE,
                               1000, &state));

  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys1, h1));

  // 2 SystemImpls are leaked...
}

TEST(SystemImplTest, BasicDataPipe) {
  MojoSystemImpl sys0 = MojoSystemImplCreateImpl();
  MojoSystemImpl sys1 = MojoSystemImplCreateImpl();
  EXPECT_NE(sys0, sys1);

  MojoHandle hp, hc;
  MojoHandleSignals sig;
  char buffer[20] = {0};
  uint32_t buffer_size;
  void* write_pointer;
  const void* read_pointer;

  hp = MOJO_HANDLE_INVALID;
  hc = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplCreateDataPipe(sys0, nullptr, &hp, &hc));
  EXPECT_NE(hp, MOJO_HANDLE_INVALID);
  EXPECT_NE(hc, MOJO_HANDLE_INVALID);
  EXPECT_NE(hp, hc);

  // Both handles should have the correct rights.
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetRights(sys0, hp, &rights));
  EXPECT_EQ(kDefaultDataPipeProducerHandleRights, rights);
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetRights(sys0, hc, &rights));
  EXPECT_EQ(kDefaultDataPipeConsumerHandleRights, rights);

  // Shouldn't be able to duplicate either handle (just test "with reduced
  // rights" on one, and without on the other).
  MojoHandle handle_denied = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED,
            MojoSystemImplDuplicateHandleWithReducedRights(
                sys0, hp, MOJO_HANDLE_RIGHT_DUPLICATE, &handle_denied));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_denied);
  handle_denied = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED,
            MojoSystemImplDuplicateHandle(sys0, hc, &handle_denied));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_denied);

  // Move the other end of the pipe to a different SystemImpl.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplTransferHandle(sys0, hc, sys1, &hc));
  EXPECT_NE(hc, MOJO_HANDLE_INVALID);

  // The consumer |hc| shouldn't be readable.
  MojoHandleSignalsState state;
  EXPECT_EQ(
      MOJO_RESULT_DEADLINE_EXCEEDED,
      MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READABLE, 0, &state));

  EXPECT_EQ(MOJO_HANDLE_SIGNAL_NONE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // The producer |hp| should be writable.
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoSystemImplWait(sys0, hp, MOJO_HANDLE_SIGNAL_WRITABLE, 0, &state));

  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfiable_signals);

  // Try to read from |hc|.
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_SHOULD_WAIT,
            MojoSystemImplReadData(sys1, hc, buffer, &buffer_size,
                                   MOJO_READ_DATA_FLAG_NONE));

  // Try to begin a two-phase read from |hc|.
  read_pointer = nullptr;
  EXPECT_EQ(MOJO_RESULT_SHOULD_WAIT,
            MojoSystemImplBeginReadData(sys1, hc, &read_pointer, &buffer_size,
                                        MOJO_READ_DATA_FLAG_NONE));

  // Write to |hp|.
  static const char kHello[] = "hello ";
  // Don't include terminating null.
  buffer_size = static_cast<uint32_t>(strlen(kHello));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWriteData(sys0, hp, kHello, &buffer_size,
                                    MOJO_WRITE_MESSAGE_FLAG_NONE));

  // |hc| should be(come) readable.
  uint32_t result_index = 1;
  MojoHandleSignalsState states[1];
  sig = MOJO_HANDLE_SIGNAL_READABLE;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWaitMany(sys1, &hc, &sig, 1, MOJO_DEADLINE_INDEFINITE,
                                   &result_index, states));

  EXPECT_EQ(0u, result_index);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            states[0].satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            states[0].satisfiable_signals);

  // Do a two-phase write to |hp|.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplBeginWriteData(sys0, hp, &write_pointer, &buffer_size,
                                         MOJO_WRITE_DATA_FLAG_NONE));
  static const char kWorld[] = "world";
  ASSERT_GE(buffer_size, sizeof(kWorld));
  // Include the terminating null.
  memcpy(write_pointer, kWorld, sizeof(kWorld));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplEndWriteData(sys0, hp,
                                       static_cast<uint32_t>(sizeof(kWorld))));

  // Read one character from |hc|.
  memset(buffer, 0, sizeof(buffer));
  buffer_size = 1;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplReadData(sys1, hc, buffer, &buffer_size,
                                   MOJO_READ_DATA_FLAG_NONE));

  // Close |hp|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys0, hp));

  // |hc| should still be readable.
  EXPECT_EQ(
      MOJO_RESULT_OK,
      MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READABLE, 0, &state));

  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // Do a two-phase read from |hc|.
  read_pointer = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplBeginReadData(sys1, hc, &read_pointer, &buffer_size,
                                        MOJO_READ_DATA_FLAG_NONE));
  ASSERT_LE(buffer_size, sizeof(buffer) - 1);
  memcpy(&buffer[1], read_pointer, buffer_size);
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplEndReadData(sys1, hc, buffer_size));
  EXPECT_STREQ("hello world", buffer);

  // |hc| should no longer be readable.
  EXPECT_EQ(
      MOJO_RESULT_FAILED_PRECONDITION,
      MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READABLE, 1000, &state));

  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys1, hc));

  // TODO(vtl): Test the other way around -- closing the consumer should make
  // the producer never-writable?

  // 2 SystemImpls are leaked...
}

TEST(SystemImplTest, DataPipeWriteThreshold) {
  MojoSystemImpl sys0 = MojoSystemImplCreateImpl();
  MojoSystemImpl sys1 = MojoSystemImplCreateImpl();
  EXPECT_NE(sys0, sys1);

  const MojoCreateDataPipeOptions options = {
      static_cast<uint32_t>(
          sizeof(MojoCreateDataPipeOptions)),   // |struct_size|.
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
      2u,                                       // |element_num_bytes|.
      4u                                        // |capacity_num_bytes|.
  };
  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplCreateDataPipe(sys0, &options, &hp, &hc));
  EXPECT_NE(hp, MOJO_HANDLE_INVALID);
  EXPECT_NE(hc, MOJO_HANDLE_INVALID);
  EXPECT_NE(hc, hp);

  // Move the other end of the pipe to a different SystemImpl.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplTransferHandle(sys0, hc, sys1, &hc));
  EXPECT_NE(hc, MOJO_HANDLE_INVALID);

  MojoDataPipeProducerOptions popts;
  static const uint32_t kPoptsSize = static_cast<uint32_t>(sizeof(popts));

  // Check the current write threshold; should be the default.
  memset(&popts, 255, kPoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetDataPipeProducerOptions(
                                sys0, hp, &popts, kPoptsSize));
  EXPECT_EQ(kPoptsSize, popts.struct_size);
  EXPECT_EQ(0u, popts.write_threshold_num_bytes);

  // Should already have the write threshold signal.
  MojoHandleSignalsState state = {};
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWait(sys0, hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0,
                               &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfiable_signals);

  // Try setting the write threshold to something invalid.
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoSystemImplSetDataPipeProducerOptions(sys0, hp, &popts));
  // It shouldn't change the options.
  memset(&popts, 255, kPoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetDataPipeProducerOptions(
                                sys0, hp, &popts, kPoptsSize));
  EXPECT_EQ(kPoptsSize, popts.struct_size);
  EXPECT_EQ(0u, popts.write_threshold_num_bytes);

  // Write an element.
  static const uint16_t kTestElem = 12345u;
  uint32_t num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWriteData(sys0, hp, &kTestElem, &num_bytes,
                                    MOJO_WRITE_MESSAGE_FLAG_NONE));
  EXPECT_EQ(2u, num_bytes);

  // Should still have the write threshold signal.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWait(sys0, hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0,
                               nullptr));

  // Write another element.
  static const uint16_t kAnotherTestElem = 12345u;
  num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWriteData(sys0, hp, &kAnotherTestElem, &num_bytes,
                                    MOJO_WRITE_MESSAGE_FLAG_NONE));
  EXPECT_EQ(2u, num_bytes);

  // Should no longer have the write threshold signal.
  state = MojoHandleSignalsState();
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoSystemImplWait(sys0, hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0,
                               &state));
  EXPECT_EQ(0u, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfiable_signals);

  // Set the write threshold to 2 (one element).
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplSetDataPipeProducerOptions(sys0, hp, &popts));
  // It should actually change the options.
  memset(&popts, 255, kPoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetDataPipeProducerOptions(
                                sys0, hp, &popts, kPoptsSize));
  EXPECT_EQ(kPoptsSize, popts.struct_size);
  EXPECT_EQ(2u, popts.write_threshold_num_bytes);

  // Should still not have the write threshold signal.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoSystemImplWait(sys0, hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0,
                               nullptr));

  // Read an element.
  uint16_t read_elem = 0u;
  num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplReadData(sys1, hc, &read_elem, &num_bytes,
                                   MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(2u, num_bytes);
  EXPECT_EQ(kTestElem, read_elem);

  // Should get the write threshold signal now.
  state = MojoHandleSignalsState();
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWait(sys0, hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
                               1000, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD,
            state.satisfiable_signals);

  // Set the write threshold to 4 (two elements).
  popts.struct_size = kPoptsSize;
  popts.write_threshold_num_bytes = 4u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplSetDataPipeProducerOptions(sys0, hp, &popts));

  // Should again not have the write threshold signal.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoSystemImplWait(sys0, hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0,
                               nullptr));

  // Close the consumer.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys1, hc));

  // The write threshold signal should now be unsatisfiable.
  state = MojoHandleSignalsState();
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoSystemImplWait(sys0, hp, MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD, 0,
                               &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys0, hp));
}

TEST(SystemImplTest, DataPipeReadThreshold) {
  MojoSystemImpl sys0 = MojoSystemImplCreateImpl();
  MojoSystemImpl sys1 = MojoSystemImplCreateImpl();
  EXPECT_NE(sys0, sys1);

  MojoHandle hp = MOJO_HANDLE_INVALID;
  MojoHandle hc = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplCreateDataPipe(sys0, nullptr, &hp, &hc));
  EXPECT_NE(hp, MOJO_HANDLE_INVALID);
  EXPECT_NE(hc, MOJO_HANDLE_INVALID);

  // Move the other end of the pipe to a different SystemImpl.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplTransferHandle(sys0, hc, sys1, &hc));
  EXPECT_NE(hc, MOJO_HANDLE_INVALID);

  MojoDataPipeConsumerOptions copts;
  static const uint32_t kCoptsSize = static_cast<uint32_t>(sizeof(copts));

  // Check the current read threshold; should be the default.
  memset(&copts, 255, kCoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetDataPipeConsumerOptions(
                                sys1, hc, &copts, kCoptsSize));
  EXPECT_EQ(kCoptsSize, copts.struct_size);
  EXPECT_EQ(0u, copts.read_threshold_num_bytes);

  // Shouldn't have the read threshold signal yet.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
                               1000, nullptr));

  // Write a byte to |hp|.
  static const char kAByte = 'A';
  uint32_t num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWriteData(sys0, hp, &kAByte, &num_bytes,
                                    MOJO_WRITE_MESSAGE_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);

  // Now should have the read threshold signal.
  MojoHandleSignalsState state;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
                               1000, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // Set the read threshold to 3, and then check it.
  copts.struct_size = kCoptsSize;
  copts.read_threshold_num_bytes = 3u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplSetDataPipeConsumerOptions(sys1, hc, &copts));

  memset(&copts, 255, kCoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetDataPipeConsumerOptions(
                                sys1, hc, &copts, kCoptsSize));
  EXPECT_EQ(kCoptsSize, copts.struct_size);
  EXPECT_EQ(3u, copts.read_threshold_num_bytes);

  // Shouldn't have the read threshold signal again.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0,
                               nullptr));

  // Write another byte to |hp|.
  static const char kBByte = 'B';
  num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWriteData(sys0, hp, &kBByte, &num_bytes,
                                    MOJO_WRITE_MESSAGE_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);

  // Still shouldn't have the read threshold signal.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
                               1000, nullptr));

  // Write a third byte to |hp|.
  static const char kCByte = 'C';
  num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWriteData(sys0, hp, &kCByte, &num_bytes,
                                    MOJO_WRITE_MESSAGE_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);

  // Now should have the read threshold signal.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
                               1000, nullptr));

  // Read a byte.
  char read_byte = 'x';
  num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplReadData(sys1, hc, &read_byte, &num_bytes,
                                   MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);
  EXPECT_EQ(kAByte, read_byte);

  // Shouldn't have the read threshold signal again.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0,
                               nullptr));

  // Set the read threshold to 2.
  copts.struct_size = kCoptsSize;
  copts.read_threshold_num_bytes = 2u;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplSetDataPipeConsumerOptions(sys1, hc, &copts));

  // Should have the read threshold signal again.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0,
                               nullptr));

  // Set the read threshold to the default by passing null, and check it.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplSetDataPipeConsumerOptions(sys1, hc, nullptr));

  memset(&copts, 255, kCoptsSize);
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetDataPipeConsumerOptions(
                                sys1, hc, &copts, kCoptsSize));
  EXPECT_EQ(kCoptsSize, copts.struct_size);
  EXPECT_EQ(0u, copts.read_threshold_num_bytes);

  // Should still have the read threshold signal.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplWait(sys1, hc, MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0,
                               nullptr));

  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys0, hp));
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys1, hc));

  // 2 SystemImpls are leaked...
}

TEST(SystemImplTest, BasicSharedBuffer) {
  const uint64_t kSize = 100u;

  MojoSystemImpl sys0 = MojoSystemImplCreateImpl();
  MojoSystemImpl sys1 = MojoSystemImplCreateImpl();
  EXPECT_NE(sys0, sys1);

  // Create a shared buffer (|h0|).
  MojoHandle h0 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplCreateSharedBuffer(sys0, nullptr, kSize, &h0));
  EXPECT_NE(h0, MOJO_HANDLE_INVALID);

  // The handle should have the correct rights.
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetRights(sys0, h0, &rights));
  EXPECT_EQ(kDefaultSharedBufferHandleRights, rights);

  // Check the buffer information.
  {
    MojoBufferInformation info = {};
    EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetBufferInformation(
                                  sys0, h0, &info, sizeof(info)));
    EXPECT_EQ(sizeof(info), info.struct_size);
    EXPECT_EQ(MOJO_BUFFER_INFORMATION_FLAG_NONE, info.flags);
    EXPECT_EQ(kSize, info.num_bytes);
  }

  // Map everything.
  void* pointer = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplMapBuffer(sys0, h0, 0u, kSize, &pointer,
                                    MOJO_MAP_BUFFER_FLAG_NONE));
  ASSERT_TRUE(pointer);
  static_cast<char*>(pointer)[kSize / 2] = 'x';

  // Duplicate |h0| to |h1|.
  MojoHandle h1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplDuplicateHandle(sys0, h0, &h1));
  EXPECT_NE(h1, MOJO_HANDLE_INVALID);
  EXPECT_NE(h1, h0);

  // The new handle should have the correct rights.
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetRights(sys0, h1, &rights));
  EXPECT_EQ(kDefaultSharedBufferHandleRights, rights);

  // Move the other end of the pipe to a different SystemImpl.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplTransferHandle(sys0, h1, sys1, &h1));
  EXPECT_NE(h1, MOJO_HANDLE_INVALID);

  // Close |h0|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys0, h0));

  // The mapping should still be good.
  static_cast<char*>(pointer)[kSize / 2 + 1] = 'y';

  // Unmap it.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplUnmapBuffer(sys0, pointer));

  // Check the buffer information on |h1|.
  {
    MojoBufferInformation info = {};
    EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetBufferInformation(
                                  sys1, h1, &info, sizeof(info)));
    EXPECT_EQ(sizeof(info), info.struct_size);
    EXPECT_EQ(MOJO_BUFFER_INFORMATION_FLAG_NONE, info.flags);
    EXPECT_EQ(kSize, info.num_bytes);
  }

  // Map half of |h1|.
  pointer = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSystemImplMapBuffer(sys1, h1, kSize / 2, kSize / 2, &pointer,
                                    MOJO_MAP_BUFFER_FLAG_NONE));
  ASSERT_TRUE(pointer);

  // It should have what we wrote.
  EXPECT_EQ('x', static_cast<char*>(pointer)[0]);
  EXPECT_EQ('y', static_cast<char*>(pointer)[1]);

  // Unmap it.
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplUnmapBuffer(sys1, pointer));

  // Test duplication with reduced rights.
  MojoHandle h2 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplDuplicateHandleWithReducedRights(
                                sys1, h1, MOJO_HANDLE_RIGHT_DUPLICATE, &h2));
  EXPECT_NE(h2, MOJO_HANDLE_INVALID);
  EXPECT_NE(h2, h1);
  // |h2| should have the correct rights.
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplGetRights(sys1, h2, &rights));
  EXPECT_EQ(kDefaultSharedBufferHandleRights & ~MOJO_HANDLE_RIGHT_DUPLICATE,
            rights);
  // Trying to duplicate |h2| should fail.
  MojoHandle h3 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED,
            MojoSystemImplDuplicateHandle(sys1, h2, &h3));
  EXPECT_EQ(MOJO_HANDLE_INVALID, h3);

  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys1, h1));
  EXPECT_EQ(MOJO_RESULT_OK, MojoSystemImplClose(sys1, h2));

  // 2 SystemImpls are leaked...
}

}  // namespace
}  // namespace mojo
