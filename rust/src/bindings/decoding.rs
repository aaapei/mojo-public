// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use bindings::encoding::{Bits, Context, MojomNumeric};
use bindings::mojom::{MOJOM_NULL_POINTER, UNION_SIZE};
use bindings::util;

use std::mem;
use std::ptr;
use std::vec::Vec;

use system;
use system::{Handle, CastHandle, UntypedHandle};

/// An decoding state represents the decoding logic for a single
/// Mojom object that is NOT inlined, such as a struct or an array.
pub struct DecodingState<'slice> {
    /// The buffer the state may write to.
    data: &'slice [u8],

    /// The offset of this serialized object into the overall buffer.
    global_offset: usize,

    /// The current offset within 'data'.
    offset: usize,

    /// The current bit offset within 'data'.
    bit_offset: Bits,
}

impl<'slice> DecodingState<'slice> {
    /// Create a new decoding state.
    pub fn new(buffer: &'slice [u8], offset: usize) -> DecodingState<'slice> {
        DecodingState {
            data: buffer,
            global_offset: offset,
            offset: 0,
            bit_offset: Bits(0),
        }
    }

    /// Align the decoding state to the next byte.
    pub fn align_to_byte(&mut self) {
        if self.bit_offset > Bits(0) {
            self.offset += 1;
            self.bit_offset = Bits(0);
        }
    }

    /// Align the decoding state to the next 'bytes' boundary.
    pub fn align_to_bytes(&mut self, bytes: usize) {
        if self.offset != 0 {
            self.offset = util::align_bytes(self.offset, bytes);
        }
    }

    /// Read a primitive from the buffer without incrementing the offset.
    fn read_in_place<T: MojomNumeric>(&mut self) -> T {
        let mut value: T = Default::default();
        debug_assert!(mem::size_of::<T>() + self.offset <= self.data.len());
        let ptr = (&self.data[self.offset..]).as_ptr();
        unsafe {
            ptr::copy_nonoverlapping(mem::transmute::<*const u8, *const T>(ptr),
                                     &mut value as *mut T,
                                     1);
        }
        value
    }

    /// Read a primitive from the buffer and increment the offset.
    fn read<T: MojomNumeric>(&mut self) -> T {
        let value = self.read_in_place::<T>();
        self.bit_offset = Bits(0);
        self.offset += mem::size_of::<T>();
        value
    }

    /// Decode a primitive from the buffer, naturally aligning before we read.
    pub fn decode<T: MojomNumeric>(&mut self) -> T {
        self.align_to_byte();
        self.align_to_bytes(mem::size_of::<T>());
        self.read::<T>()
    }

    /// Decode a boolean value from the buffer as one bit.
    pub fn decode_bool(&mut self) -> bool {
        let offset = self.offset;
        // Check the bit by getting the set bit and checking if its non-zero
        let value = (self.data[offset] & self.bit_offset.as_set_bit()) > 0;
        self.bit_offset += Bits(1);
        let (bits, bytes) = self.bit_offset.as_bits_and_bytes();
        self.offset += bytes;
        self.bit_offset = bits;
        value
    }

    /// If we encounter a null pointer, increment past it.
    ///
    /// Returns if we skipped or not.
    pub fn skip_if_null_pointer(&mut self) -> bool {
        self.align_to_byte();
        self.align_to_bytes(8);
        let ptr = self.read_in_place::<u64>();
        if ptr == MOJOM_NULL_POINTER {
            self.offset += 8;
        }
        (ptr == MOJOM_NULL_POINTER)
    }

    /// If we encounter a null union, increment past it.
    ///
    /// Returns if we skipped or not.
    pub fn skip_if_null_union(&mut self) -> bool {
        self.align_to_byte();
        self.align_to_bytes(8);
        let size = self.read_in_place::<u32>();
        if size == 0 {
            self.offset += UNION_SIZE;
        }
        (size == 0)
    }

    /// If we encounter a null handle, increment past it.
    ///
    /// Returns if we skipped or not.
    pub fn skip_if_null_handle(&mut self) -> bool {
        self.align_to_byte();
        self.align_to_bytes(4);
        let index = self.read_in_place::<i32>();
        if index < 0 {
            self.offset += 4;
        }
        (index < 0)
    }

    /// If we encounter a null interface, increment past it.
    ///
    /// Returns if we skipped or not.
    pub fn skip_if_null_interface(&mut self) -> bool {
        self.align_to_byte();
        self.align_to_bytes(4);
        let index = self.read_in_place::<i32>();
        if index < 0 {
            self.offset += 8;
        }
        (index < 0)
    }

    /// Decode a pointer from the buffer as a global offset into the buffer.
    ///
    /// The pointer in the buffer is an offset relative to the pointer to another
    /// location in the buffer. We convert that to an absolute offset with respect
    /// to the buffer before returning. This is our defintion of a pointer.
    pub fn decode_pointer(&mut self) -> u64 {
        self.align_to_byte();
        self.align_to_bytes(8);
        let current_location = (self.global_offset + self.offset) as u64;
        self.read::<u64>() + current_location
    }
}

/// A struct that will encode a given Mojom object and convert it into
/// bytes and a vector of handles.
pub struct Decoder<'slice> {
    bytes: usize,
    buffer: Option<&'slice [u8]>,
    states: Vec<DecodingState<'slice>>,
    handles: Vec<UntypedHandle>,
}

impl<'slice> Decoder<'slice> {
    /// Create a new Decoder.
    pub fn new(buffer: &'slice [u8], handles: Vec<UntypedHandle>) -> Decoder<'slice> {
        Decoder {
            bytes: 0,
            buffer: Some(buffer),
            states: Vec::new(),
            handles: handles,
        }
    }

    /// Claim space in the buffer to start decoding some object.
    ///
    /// Creates a new decoding state for the object and returns a context.
    pub fn claim(&mut self, offset: usize) -> Option<Context> {
        if offset < self.bytes {
            panic!("Tried to claim already-claimed space!");
        }
        let mut buffer = self.buffer.take().expect("No buffer?");
        let space = offset - self.bytes;
        buffer = &buffer[space..];
        let mut read_size: u32 = 0;
        unsafe {
            ptr::copy_nonoverlapping(mem::transmute::<*const u8, *const u32>(buffer.as_ptr()),
                                     &mut read_size as *mut u32,
                                     mem::size_of::<u32>());
        }
        let size = u32::from_le(read_size) as usize;
        // TODO(mknyszek): Check size for validation
        let (claimed, unclaimed) = buffer.split_at(size);
        self.states.push(DecodingState::new(claimed, offset));
        self.buffer = Some(unclaimed);
        self.bytes += space + size;
        Some(Context::new(self.states.len() - 1))
    }

    /// Claims a handle at some particular index in the given handles array.
    ///
    /// Returns the handle with all type information in-tact.
    pub fn claim_handle<T: Handle + CastHandle>(&mut self, index: i32) -> T {
        let real_index = if index >= 0 {
            index as usize
        } else {
            panic!("Tried to claim null handle!");
        };
        // TODO(mknyszek): Check to make sure index is valid
        let raw_handle = self.handles[real_index].get_native_handle();
        unsafe {
            // TODO(mknyszek): Return an error instead of panicking
            self.handles[real_index].invalidate();
            T::from_untyped(system::acquire(raw_handle))
        }
    }

    /// Immutably borrow a decoding state via Context.
    pub fn get(&self, context: &Context) -> &DecodingState<'slice> {
        &self.states[context.id()]
    }

    /// Mutably borrow a decoding state via Context.
    pub fn get_mut(&mut self, context: &Context) -> &mut DecodingState<'slice> {
        &mut self.states[context.id()]
    }
}
