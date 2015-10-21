// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/message_header_validator.h"

#include "mojo/public/cpp/bindings/lib/bounds_checker.h"
#include "mojo/public/cpp/bindings/lib/message_validation.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/bindings/lib/validation_util.h"
#include "mojo/public/interfaces/bindings/interface_control_messages.mojom.h"

namespace mojo {
namespace internal {
namespace {

bool IsValidMessageHeader(const MessageHeader* header) {
  // NOTE: Our goal is to preserve support for future extension of the message
  // header. If we encounter fields we do not understand, we must ignore them.

  // Extra validation of the struct header:
  if (header->version == 0) {
    if (header->num_bytes != sizeof(MessageHeader)) {
      ReportValidationError(ValidationError::UNEXPECTED_STRUCT_HEADER);
      return false;
    }
  } else if (header->version == 1) {
    if (header->num_bytes != sizeof(MessageHeaderWithRequestID)) {
      ReportValidationError(ValidationError::UNEXPECTED_STRUCT_HEADER);
      return false;
    }
  } else if (header->version > 1) {
    if (header->num_bytes < sizeof(MessageHeaderWithRequestID)) {
      ReportValidationError(ValidationError::UNEXPECTED_STRUCT_HEADER);
      return false;
    }
  }

  // Validate flags (allow unknown bits):

  // These flags require a RequestID.
  if (header->version < 1 && ((header->flags & kMessageExpectsResponse) ||
                              (header->flags & kMessageIsResponse))) {
    ReportValidationError(ValidationError::MESSAGE_HEADER_MISSING_REQUEST_ID);
    return false;
  }

  // These flags are mutually exclusive.
  if ((header->flags & kMessageExpectsResponse) &&
      (header->flags & kMessageIsResponse)) {
    ReportValidationError(ValidationError::MESSAGE_HEADER_INVALID_FLAGS);
    return false;
  }

  return true;
}

}  // namespace

MessageHeaderValidator::MessageHeaderValidator(MessageReceiver* sink)
    : MessageFilter(sink) {
}

bool MessageHeaderValidator::Accept(Message* message) {
  // Pass 0 as number of handles because we don't expect any in the header, even
  // if |message| contains handles.
  BoundsChecker bounds_checker(message->data(), message->data_num_bytes(), 0);

  if (!ValidateStructHeaderAndClaimMemory(message->data(), &bounds_checker))
    return false;

  if (!IsValidMessageHeader(message->header()))
    return false;

  return sink_->Accept(message);
}

bool ValidateControlRequest(const Message* message) {
  switch (message->header()->name) {
    case kRunMessageId:
      return ValidateMessageIsRequestExpectingResponse(message) &&
             ValidateMessagePayload<RunMessageParams_Data>(message);
    case kRunOrClosePipeMessageId:
      return ValidateMessageIsRequestWithoutResponse(message) &&
             ValidateMessagePayload<RunOrClosePipeMessageParams_Data>(message);
  }
  return false;
}

bool ValidateControlResponse(const Message* message) {
  if (!ValidateMessageIsResponse(message))
    return false;
  switch (message->header()->name) {
    case kRunMessageId:
      return ValidateMessagePayload<RunResponseMessageParams_Data>(message);
  }
  return false;
}

}  // namespace internal
}  // namespace mojo
