// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use bindings::encoding;
use bindings::encoding::{Bits, Encoder, Context, DATA_HEADER_SIZE, DataHeader, DataHeaderValue};
use bindings::message::MessageHeader;

use std::cmp::Eq;
use std::collections::HashMap;
use std::hash::Hash;
use std::mem;
use std::vec::Vec;

use system::{CastHandle, Handle, UntypedHandle};
use system::data_pipe;
use system::message_pipe;
use system::shared_buffer;
use system::wait_set;

/// The size of a Mojom map plus header in bytes.
const MAP_SIZE: usize = 24;

/// The size of a Mojom union in bytes (header included).
pub const UNION_SIZE: usize = 16;

/// The size of a Mojom pointer in bits.
pub const POINTER_BIT_SIZE: Bits = Bits(64);

/// The value of a Mojom null pointer.
pub const MOJOM_NULL_POINTER: u64 = 0;

/// An enumeration of all the possible low-level Mojom types.
pub enum MojomType {
    Simple,
    Pointer,
    Union,
    Handle,
    Interface,
}

/// Whatever implements this trait can be serialized in the Mojom format.
pub trait MojomEncodable: Sized {
    /// Get the Mojom type.
    fn mojom_type() -> MojomType;

    /// Get this type's Mojom alignment.
    fn mojom_alignment() -> usize;

    /// The amount of space in bits the type takes up when inlined
    /// into another type at serialization time.
    fn embed_size(context: &Context) -> Bits;

    /// Recursively computes the size of the complete Mojom archive
    /// starting from this type.
    fn compute_size(&self, context: Context) -> usize;

    /// Encodes this type into the encoder given a context.
    fn encode(self, encoder: &mut Encoder, context: Context);
}

/// Whatever implements this trait is a Mojom pointer type which means
/// that on encode, a pointer is inlined and the implementer is
/// serialized elsewhere in the output buffer.
pub trait MojomPointer: MojomEncodable {
    /// Get the DataHeader meta-data for this pointer type.
    fn header_data(&self) -> DataHeaderValue;

    /// Get the size of only this type when serialized.
    fn serialized_size(&self, context: &Context) -> usize;

    /// Encodes the actual values of the type into the encoder.
    fn encode_value(self, encoder: &mut Encoder, context: Context);

    /// Writes a pointer inlined into the current context before calling
    /// encode_value.
    fn encode_new(self, encoder: &mut Encoder, context: Context) {
        let data_size = self.serialized_size(&context);
        let data_header = DataHeader::new(data_size, self.header_data());
        let new_context = encoder.add(&data_header).unwrap();
        self.encode_value(encoder, new_context);
    }
}

/// Whatever implements this trait is a Mojom union type which means that
/// on encode it is inlined, but if the union is nested inside of another
/// union type, it is treated as a pointer type.
pub trait MojomUnion: MojomEncodable {
    /// Get the union's current tag.
    fn get_tag(&self) -> u32;

    /// Encode the actual value of the union.
    fn encode_value(self, encoder: &mut Encoder, context: Context);

    /// The embed_size for when the union is a pointer type.
    fn nested_embed_size() -> Bits {
        POINTER_BIT_SIZE
    }

    /// The encoding routine for when the union is a pointer type.
    fn nested_encode(self, encoder: &mut Encoder, context: Context) {
        let loc = encoder.size() as u64;
        {
            let state = encoder.get_mut(&context);
            state.encode_pointer(loc);
        }
        let tag = DataHeaderValue::UnionTag(self.get_tag());
        let data_header = DataHeader::new(UNION_SIZE, tag);
        let new_context = encoder.add(&data_header).unwrap();
        self.encode_value(encoder, new_context.set_is_union(true));
    }

    /// The embed_size for when the union is inlined into the current context.
    fn inline_embed_size() -> Bits {
        Bits(8 * (UNION_SIZE as usize))
    }

    /// The encoding routine for when the union is inlined into the current context.
    fn inline_encode(self, encoder: &mut Encoder, context: Context) {
        {
            let mut state = encoder.get_mut(&context);
            state.align_to_bytes(8);
            state.encode(UNION_SIZE as u32);
            state.encode(self.get_tag());
        }
        self.encode_value(encoder, context.clone());
        {
            let mut state = encoder.get_mut(&context);
            state.align_to_bytes(8);
            state.align_to_byte();
        }
    }
}

/// A marker trait that marks Mojo handles as encodable.
pub trait MojomHandle: CastHandle + MojomEncodable {}

/// Whatever implements this trait is considered to be a Mojom
/// interface, that is, a message pipe which conforms to some
/// messaging interface.
///
/// We force an underlying message pipe to be used via the pipe()
/// and unwrap() routines.
pub trait MojomInterface: MojomEncodable {
    /// Get the service name for this interface.
    fn service_name() -> &'static str;

    /// Get the version for this interface.
    fn version() -> u32;

    /// Access the underlying message pipe for this interface.
    fn pipe(&self) -> &message_pipe::MessageEndpoint;

    /// Unwrap the interface into its underlying message pipe.
    fn unwrap(self) -> message_pipe::MessageEndpoint;
}

/// Whatever implements this trait is considered to be a Mojom
/// interface that may send messages of some generic type.
///
/// When implementing this trait, the correct way is to specify
/// a tighter trait bound than MojomMessage that limits the types
/// available for sending to those that are valid messages available
/// to the interface.
pub trait MojomInterfaceSend<R: MojomMessage>: MojomInterface {
    /// Creates a message.
    fn create_request(&self, payload: R) -> (Vec<u8>, Vec<UntypedHandle>) {
        let header = R::create_header();
        // TODO(mknyszek): Calculate and set an actual request id
        let header_size = header.compute_size(Default::default());
        let size = header_size + payload.compute_size(Default::default());
        let mut buffer: Vec<u8> = Vec::with_capacity(size);
        buffer.resize(size, 0);
        let handles = {
            let (header_buf, rest_buf) = buffer.split_at_mut(header_size);
            let mut handles = header.serialize(header_buf);
            handles.extend(payload.serialize(rest_buf).into_iter());
            handles
        };
        (buffer, handles)
    }

    /// Creates and sends a message.
    fn send_request(&self, payload: R) {
        let (buffer, handles) = self.create_request(payload);
        self.pipe().write(&buffer, handles, mpflags!(Write::None));
    }
}

/// Whatever implements this trait is considered to be a Mojom struct.
///
/// Mojom structs are always the root of any Mojom message. Thus, we
/// provide convenience functions for serialization here.
pub trait MojomStruct: MojomPointer {
    /// Given a pre-allocated buffer, the struct serializes itself.
    fn serialize(self, buffer: &mut [u8]) -> Vec<UntypedHandle> {
        let mut encoder = Encoder::new(buffer);
        self.encode_new(&mut encoder, Default::default());
        encoder.unwrap()
    }

    /// The struct computes its own size, allocates a buffer, and then
    /// serializes itself into that buffer.
    fn auto_serialize(self) -> (Vec<u8>, Vec<UntypedHandle>) {
        let size = self.compute_size(Default::default());
        let mut buf = Vec::with_capacity(size);
        buf.resize(size, 0);
        let handles = self.serialize(&mut buf);
        (buf, handles)
    }
}

/// Marks a MojomStruct as being capable of being sent across some
/// Mojom interface.
pub trait MojomMessage: MojomStruct {
    fn create_header() -> MessageHeader;
}

// ********************************************** //
// ****** IMPLEMENTATIONS FOR COMMON TYPES ****** //
// ********************************************** //

macro_rules! impl_encodable_for_prim {
    ($($prim_type:ty),*) => {
        $(
        impl MojomEncodable for $prim_type {
            fn mojom_type() -> MojomType {
                MojomType::Simple
            }
            fn mojom_alignment() -> usize {
                mem::size_of::<$prim_type>()
            }
            fn embed_size(_context: &Context) -> Bits {
                Bits(8 * mem::size_of::<$prim_type>())
            }
            fn compute_size(&self, _context: Context) -> usize {
                0 // Indicates that this type is inlined and it adds nothing external to the size
            }
            fn encode(self, encoder: &mut Encoder, context: Context) {
                let mut state = encoder.get_mut(&context);
                state.encode(self);
            }
        }
        )*
    }
}

impl_encodable_for_prim!(i8, i16, i32, i64, u8, u16, u32, u64, f32, f64);

impl MojomEncodable for bool {
    fn mojom_alignment() -> usize {
        panic!("Should never check mojom_alignment of bools (they're bit-aligned)!");
    }
    fn mojom_type() -> MojomType {
        MojomType::Simple
    }
    fn embed_size(_context: &Context) -> Bits {
        Bits(1)
    }
    fn compute_size(&self, _context: Context) -> usize {
        0 // Indicates that this type is inlined and it adds nothing external to the size
    }
    fn encode(self, encoder: &mut Encoder, context: Context) {
        let mut state = encoder.get_mut(&context);
        state.encode_bool(self);
    }
}

// Options should be considered to represent nullability the Mojom IDL.
// Any type wrapped in an Option type is nullable.

impl<T: MojomEncodable> MojomEncodable for Option<T> {
    fn mojom_alignment() -> usize {
        T::mojom_alignment()
    }
    fn mojom_type() -> MojomType {
        T::mojom_type()
    }
    fn embed_size(context: &Context) -> Bits {
        T::embed_size(context)
    }
    fn compute_size(&self, context: Context) -> usize {
        match *self {
            Some(ref value) => value.compute_size(context),
            None => 0,
        }
    }
    fn encode(self, encoder: &mut Encoder, context: Context) {
        match self {
            Some(value) => value.encode(encoder, context),
            None => {
                let mut state = encoder.get_mut(&context);
                match T::mojom_type() {
                    MojomType::Pointer => state.encode_pointer(MOJOM_NULL_POINTER),
                    MojomType::Union => state.encode_null_union(),
                    MojomType::Handle => state.encode(-1 as i32),
                    MojomType::Interface => {
                        state.encode(-1 as i32);
                        state.encode(0 as u32);
                    },
                    MojomType::Simple => panic!("Unexpected simple type in Option!"),
                }
            },
        }
    }
}

macro_rules! impl_pointer_for_array {
    () => {
        fn header_data(&self) -> DataHeaderValue {
            DataHeaderValue::Elements(self.len() as u32)
        }
        fn serialized_size(&self, context: &Context) -> usize {
            DATA_HEADER_SIZE + if self.len() > 0 {
                (T::embed_size(context) * self.len()).as_bytes()
            } else {
                0
            }
        }
    }
}

macro_rules! impl_encodable_for_array {
    () => {
        impl_encodable_for_pointer!();
        fn compute_size(&self, context: Context) -> usize {
            let mut size = encoding::align_default(self.serialized_size(&context));
            for elem in self.iter() {
                size += elem.compute_size(context.clone());
            }
            size
        }
    }
}

impl<T: MojomEncodable> MojomPointer for Vec<T> {
    impl_pointer_for_array!();
    fn encode_value(self, encoder: &mut Encoder, context: Context) {
        for elem in self.into_iter() {
            elem.encode(encoder, context.clone());
        }
    }
}

impl<T: MojomEncodable> MojomEncodable for Vec<T> {
    impl_encodable_for_array!();
}

macro_rules! impl_encodable_for_boxed_fixed_array {
    ($($len:expr),*) => {
        $(
        impl<T: MojomEncodable> MojomPointer for Box<[T; $len]> {
            impl_pointer_for_array!();
            fn encode_value(self, encoder: &mut Encoder, context: Context) {
                for elem in (self as Box<[T]>).into_vec().into_iter() {
                    elem.encode(encoder, context.clone());
                }
            }
        }
        impl<T: MojomEncodable> MojomEncodable for Box<[T; $len]> {
            impl_encodable_for_array!();
        }
        )*
    }
}

// Unfortunately, we cannot be generic over the length of a fixed array
// even though its part of the type (this will hopefully be added in the
// future) so for now we implement encodable for only the first 33 fixed
// size array types.
impl_encodable_for_boxed_fixed_array!( 0,  1,  2,  3,  4,  5,  6,  7,
                                       8,  9, 10, 11, 12, 13, 14, 15,
                                      16, 17, 18, 19, 20, 21, 22, 23,
                                      24, 25, 26, 27, 28, 29, 30, 31,
                                      32);

impl<T: MojomEncodable> MojomPointer for Box<[T]> {
    impl_pointer_for_array!();
    fn encode_value(self, encoder: &mut Encoder, context: Context) {
        for elem in self.into_vec().into_iter() {
            elem.encode(encoder, context.clone());
        }
    }
}

impl<T: MojomEncodable> MojomEncodable for Box<[T]> {
    impl_encodable_for_array!();
}

// We can represent a Mojom string as just a Rust String type
// since both are UTF-8.
impl MojomPointer for String {
    fn header_data(&self) -> DataHeaderValue {
        DataHeaderValue::Elements(self.len() as u32)
    }
    fn serialized_size(&self, _context: &Context) -> usize {
        DATA_HEADER_SIZE + self.len()
    }
    fn encode_value(self, encoder: &mut Encoder, context: Context) {
        for byte in self.as_bytes() {
            byte.encode(encoder, context.clone());
        }
    }
}

impl MojomEncodable for String {
    impl_encodable_for_pointer!();
    fn compute_size(&self, context: Context) -> usize {
        encoding::align_default(self.serialized_size(&context))
    }
}

impl<K: MojomEncodable + Eq + Hash, V: MojomEncodable> MojomPointer for HashMap<K, V> {
    fn header_data(&self) -> DataHeaderValue {
        DataHeaderValue::Version(0)
    }
    fn serialized_size(&self, _context: &Context) -> usize {
        MAP_SIZE
    }
    fn encode_value(self, encoder: &mut Encoder, context: Context) {
        let elems = self.len();
        let meta_value = DataHeaderValue::Elements(elems as u32);
        // We need to move values into this vector because we can't copy the keys.
        // (Handles are not copyable so MojomEncodable cannot be copyable!)
        let mut vals_vec = Vec::with_capacity(elems);
        // Key setup
        // Write a pointer to the keys array.
        let keys_loc = encoder.size() as u64;
        {
            let state = encoder.get_mut(&context);
            state.encode_pointer(keys_loc);
        }
        // Create the keys data header
        let keys_bytes = DATA_HEADER_SIZE + (K::embed_size(&context) * elems).as_bytes();
        let keys_data_header = DataHeader::new(keys_bytes, meta_value);
        // Claim space for the keys array in the encoder
        let keys_context = encoder.add(&keys_data_header).unwrap();
        // Encode keys, setup vals
        for (key, value) in self.into_iter() {
            key.encode(encoder, keys_context.clone());
            vals_vec.push(value);
        }
        // Encode vals
        vals_vec.encode(encoder, context.clone())
    }
}

impl<K: MojomEncodable + Eq + Hash, V: MojomEncodable> MojomEncodable for HashMap<K, V> {
    impl_encodable_for_pointer!();
    fn compute_size(&self, context: Context) -> usize {
        let mut size = encoding::align_default(self.serialized_size(&context));
        // The size of the one array
        size += DATA_HEADER_SIZE;
        size += (K::embed_size(&context) * self.len()).as_bytes();
        size = encoding::align_default(size);
        // Any extra space used by the keys
        for (key, _) in self {
            size += key.compute_size(context.clone());
        }
        // Need to re-align after this for the next array
        size = encoding::align_default(size);
        // The size of the one array
        size += DATA_HEADER_SIZE;
        size += (V::embed_size(&context) * self.len()).as_bytes();
        size = encoding::align_default(size);
        // Any extra space used by the values
        for (_, value) in self {
            size += value.compute_size(context.clone());
        }
        // Align one more time at the end to keep the next object aligned.
        encoding::align_default(size)
    }
}

impl<T: MojomEncodable + CastHandle + Handle> MojomHandle for T {}

macro_rules! impl_encodable_for_handle {
    ($handle_type:path) => {
        fn mojom_alignment() -> usize {
            4
        }
        fn mojom_type() -> MojomType {
            MojomType::Handle
        }
        fn embed_size(_context: &Context) -> Bits {
            Bits(8 * mem::size_of::<u32>())
        }
        fn compute_size(&self, _context: Context) -> usize {
            0
        }
        fn encode(self, encoder: &mut Encoder, context: Context) {
            let pos = encoder.add_handle(self.as_untyped());
            let mut state = encoder.get_mut(&context);
            state.encode(pos as i32);
        }
    }
}

impl MojomEncodable for UntypedHandle {
    impl_encodable_for_handle!(UntypedHandle);
}

impl MojomEncodable for message_pipe::MessageEndpoint {
    impl_encodable_for_handle!(message_pipe::MessageEndpoint);
}

impl MojomEncodable for shared_buffer::SharedBuffer {
    impl_encodable_for_handle!(shared_buffer::SharedBuffer);
}

impl<T> MojomEncodable for data_pipe::Consumer<T> {
    impl_encodable_for_handle!(data_pipe::Consumer<T>);
}

impl<T> MojomEncodable for data_pipe::Producer<T> {
    impl_encodable_for_handle!(data_pipe::Producer<T>);
}

impl MojomEncodable for wait_set::WaitSet {
    impl_encodable_for_handle!(wait_set::WaitSet);
}
