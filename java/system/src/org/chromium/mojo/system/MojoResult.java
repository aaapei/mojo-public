// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.system;

/**
 * The different mojo result codes.
 *
 * TODO(vtl): Find a way of supporting the new, more flexible/extensible
 * MojoResult (see mojo/public/c/syste/result.h).
 */
public final class MojoResult {
    public static final int OK = 0x0;
    public static final int CANCELLED = 0x1;
    public static final int UNKNOWN = 0x2;
    public static final int INVALID_ARGUMENT = 0x3;
    // INVALID_ARGUMENT, subcode 0x001:
    public static final int BAD_HANDLE = 0x0013;
    // INVALID_ARGUMENT, subcode 0x002:
    public static final int WRONG_TYPE = 0x0023;
    public static final int DEADLINE_EXCEEDED = 0x4;
    public static final int NOT_FOUND = 0x5;
    public static final int ALREADY_EXISTS = 0x6;
    public static final int PERMISSION_DENIED = 0x7;
    public static final int RESOURCE_EXHAUSTED = 0x8;
    public static final int FAILED_PRECONDITION = 0x9;
    // FAILED_PRECONDITION, subcode 0x001:
    public static final int BUSY = 0x0019;
    public static final int ABORTED = 0xa;
    public static final int OUT_OF_RANGE = 0xb;
    public static final int UNIMPLEMENTED = 0xc;
    public static final int INTERNAL = 0xd;
    public static final int UNAVAILABLE = 0xe;
    // UNAVAILABLE, subcode 0x001:
    public static final int SHOULD_WAIT = 0x001e;
    public static final int DATA_LOSS = 0xf;

    /**
     * never instantiate.
     */
    private MojoResult() {
    }

    /**
     * Describes the given result code.
     */
    public static String describe(int mCode) {
        switch (mCode) {
            case OK:
                return "OK";
            case CANCELLED:
                return "CANCELLED";
            case UNKNOWN:
                return "UNKNOWN";
            case INVALID_ARGUMENT:
                return "INVALID_ARGUMENT";
            case BAD_HANDLE:
                return "BAD_HANDLE";
            case WRONG_TYPE:
                return "WRONG_TYPE";
            case DEADLINE_EXCEEDED:
                return "DEADLINE_EXCEEDED";
            case NOT_FOUND:
                return "NOT_FOUND";
            case ALREADY_EXISTS:
                return "ALREADY_EXISTS";
            case PERMISSION_DENIED:
                return "PERMISSION_DENIED";
            case RESOURCE_EXHAUSTED:
                return "RESOURCE_EXHAUSTED";
            case FAILED_PRECONDITION:
                return "FAILED_PRECONDITION";
            case BUSY:
                return "BUSY";
            case ABORTED:
                return "ABORTED";
            case OUT_OF_RANGE:
                return "OUT_OF_RANGE";
            case UNIMPLEMENTED:
                return "UNIMPLEMENTED";
            case INTERNAL:
                return "INTERNAL";
            case UNAVAILABLE:
                return "UNAVAILABLE";
            case SHOULD_WAIT:
                return "SHOULD_WAIT";
            case DATA_LOSS:
                return "DATA_LOSS";
            default:
                return "UNKNOWN";
        }

    }
}
