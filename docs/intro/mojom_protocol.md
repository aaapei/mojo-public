# Mojom protocol

The Mojom protocol is the standard protocol used by two communicating Mojo
applications. Communication is done by sending interface messages (sometimes
also called methods), whose schema is specified using the [Mojom
IDL](mojom_idl.md), of which we will assume a basic understand.

## Interface messages

Communication using the protocol happens between two parties over one
communication channel, where the requests flow one way and the responses flow
the other. Requests and response schemas are defined by mojom interface message
formats. The request and response parameters are each framed as mojom structs,
where each parameter is represented by a struct field.

The beginning of every request on the wire is framed by an interface message
header. The message header struct is versioned: one which includes a 'request
id' which is used to associate a request with a reply, and one which doesn't
include a request id. Both versions also include the 'ordinal' (a number)
identifying which interface message it is, and some flags indicating if the
message is a request expecting a response, a response, etc.

Following the message header, a mojom struct representing the request parameters
is encoded. This the top-level (i.e., the first) mojom struct that occurs on the
wire.

## Mojom struct

The wire format of a mojom struct begins with an 8-byte header, followed by
bytes representing each field. The fields on the wire appear in packing-order,
not ordinal-order (ordinal numbers are implicitly assigned to each field unless
they are explicitly assigned one in the IDL). Fields are packed in a first-first
way, and the order in which fields are packed is determined by the ordinals. A
field's byte-alignment is the same as the field's size (determined by its type);
a 4-byte type is 4-byte aligned.

## Mojom types

There are essentially two categories of mojom data types: reference (a.k.a.
pointer) types, and non-reference. In additional to data types, there are also
handle types.

### Non-reference types
These are plain-old-data types: `int8`, `uint8`, ..., `int64`, `float32`,
`float64`, and sometimes unions (unions can also be reference types -- more on
that later).

### Reference types
These are types that aren't fixed size. These types include structs, unions
(sometimes), arrays, and maps.

When a mojom struct field's type is another mojom struct, that field is encoded
as a relative byte-offset to where the actual data for the field is. The same
applies for mojom arrays and mojom maps. References can also be null, in which
case the offset is set to 0.

For example:

```mojom
struct Child {
  int32 a;
  int64 b;
  int32 c;
};
struct Parent {
  Child childA;
  Child childB;
};
```

The `Parent` struct is encoded as:
Byte Offset | Data | Type | Size |
| - | - | - | - |
0 | .. | mojom struct header for `struct Parent` | 8 |
8 | 16 | offset to `struct Child` | 8 |
16 | 32 | offset to `struct Child` | 8 |
24 | .. | mojom struct header for `struct Child` | 8 |
32 | 666 | int32 | 4 |
36 | 666 | int32 | 4 |
40 | 666 | int64 | 8 |
48 | .. | mojom struct header for `struct Child` | 8 |
56 | 666 | int32 | 4 |
60 | 666 | int32 | 4 |
64 | 666 | int64 | 8 |

Here, we see that the actually data for `childA` starts at byte `24`, so the
relative byte offset recorded is `16` (i.e., the start of the data is 16 bytes
away).

There are some restrictions in the way objects (structs, arrays, etc.) are
encoded:
1. Referenced structs must be encoded in depth-first order.
   - That is, the struct referenced by `childA` is encoded before `childB`;
     similarly, if `struct Child` were to have a reference to another struct,
     that struct must be encoded before `struct Parent`'s `childB`.
2. Referenced objects (structs, arrays, etc.) may only have 1 reference to them.
3. Offsets to objects are always positive.

These restrictions make certain operations, such as validation, cheaper.

TODO(vardhan): Write about arrays, strings, maps, unions. Maybe this can be in
mojom_lang/ instead of intro/?
TODO(vardhan): Write about how handles are encoded.
