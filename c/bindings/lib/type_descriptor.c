// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/lib/type_descriptor.h"

#include <assert.h>

#include "mojo/public/c/bindings/array.h"
#include "mojo/public/c/bindings/lib/util.h"
#include "mojo/public/c/bindings/struct.h"
#include "mojo/public/c/bindings/union.h"
#include "mojo/public/c/bindings/interface.h"

const struct MojomTypeDescriptorArray g_mojom_string_type_description = {
    MOJOM_TYPE_DESCRIPTOR_TYPE_POD,  // elem_type
    NULL,                            // elem_descriptor
    0,                               // num_elements
    false,                           // nullable
};

// The encoding of a MojoHandle is an index into an array of Handles. A
// null/invalid handle is encoded as index (which is unsigned) "-1", which
// equates to the highest possible index.
static const MojoHandle kEncodedHandleInvalid = (MojoHandle)-1;

bool MojomType_IsPointer(enum MojomTypeDescriptorType type) {
 return type == MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR ||
        type == MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR ||
        type == MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR;
}

size_t MojomType_DispatchComputeSerializedSize(
    enum MojomTypeDescriptorType type,
    const void* type_desc,
    bool nullable,
    const void* data) {
  const void* data_ptr = data;
  size_t size = 0;
  switch (type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR: {
      data_ptr = ((const union MojomPointer*)data_ptr)->ptr;
      if (!nullable || data_ptr != NULL)
        return MojomStruct_ComputeSerializedSize(
            (const struct MojomTypeDescriptorStruct*)type_desc,
            (const struct MojomStructHeader*)data_ptr);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR:
      data_ptr = ((const union MojomPointer*)data_ptr)->ptr;
      if (!nullable || data_ptr != NULL)
        return MojomArray_ComputeSerializedSize(type_desc, data_ptr);
      break;
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR:
      data_ptr = ((const union MojomPointer*)data_ptr)->ptr;
      if (data_ptr != NULL)
        size = sizeof(struct MojomUnionLayout);
      // Fall through.
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION: {
      const struct MojomUnionLayout* udata = data_ptr;
      // Unions inside unions may be set to null by setting their pointer to
      // NULL, OR by setting the union's |size| to 0.
      if (!nullable || (udata && udata->size != 0)) {
        return size + MojomUnion_ComputeSerializedSize(
            (const struct MojomTypeDescriptorUnion*)type_desc,
            (const struct MojomUnionLayout*)data);
      }
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE:
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      // We should never end up here.
      assert(false);
      break;
  }
  return size;
}

static void encode_pointer(union MojomPointer* pointer, uint32_t max_offset) {
  if (pointer->ptr == NULL) {
    pointer->offset = 0;
  } else {
    assert((char*)pointer->ptr > (char*)pointer);
    assert((char*)pointer->ptr - (char*)pointer < max_offset);
    pointer->offset = (char*)(pointer->ptr) - (char*)pointer;
  }
}

static void decode_pointer(union MojomPointer* pointer) {
  if (pointer->offset == 0) {
    pointer->ptr = NULL;
  } else {
    pointer->ptr = (char*)pointer + pointer->offset;
  }
}

static void encode_handle(bool nullable, MojoHandle* handle,
                          struct MojomHandleBuffer* handles_buffer) {
  assert(handle);
  assert(handles_buffer);
  assert(handles_buffer->handles);

  if (*handle == MOJO_HANDLE_INVALID) {
    assert(nullable);
    *handle = kEncodedHandleInvalid;
  } else {
    assert(handles_buffer->num_handles_used < handles_buffer->num_handles);

    handles_buffer->handles[handles_buffer->num_handles_used] = *handle;
    *handle = handles_buffer->num_handles_used;
    handles_buffer->num_handles_used++;
  }
}

// *handle is an index into inout_handles, or is encoded NULL.
static void decode_handle(MojoHandle* handle,
                          MojoHandle inout_handles[], uint32_t in_num_handles) {
  assert(handle);
  assert(inout_handles);

  if (*handle == kEncodedHandleInvalid) {
    *handle = MOJO_HANDLE_INVALID;
  } else {
    assert(*handle < in_num_handles);
    MojoHandle index = *handle;
    *handle = inout_handles[index];
    inout_handles[index] = MOJO_HANDLE_INVALID;
  }
}

void MojomType_DispatchEncodePointersAndHandles(
    enum MojomTypeDescriptorType in_elem_type,
    const void* in_type_desc,
    bool in_nullable,
    void* inout_buf,
    uint32_t in_buf_size,
    struct MojomHandleBuffer* inout_handles_buffer) {
  assert(inout_buf);

  void* union_buf = inout_buf;
  switch (in_elem_type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR: {
      struct MojomStructHeader* inout_struct =
          ((union MojomPointer*)inout_buf)->ptr;
      encode_pointer(inout_buf, in_buf_size);
      if (!in_nullable || inout_struct != NULL)
        MojomStruct_EncodePointersAndHandles(
            (const struct MojomTypeDescriptorStruct*)in_type_desc,
            inout_struct,
            in_buf_size - ((char*)inout_struct - (char*)inout_buf),
            inout_handles_buffer);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR: {
      struct MojomArrayHeader* inout_array =
                ((union MojomPointer*)inout_buf)->ptr;
      encode_pointer(inout_buf, in_buf_size);
      if (!in_nullable || inout_array != NULL)
        MojomArray_EncodePointersAndHandles(
            (const struct MojomTypeDescriptorArray*)in_type_desc,
            inout_array,
            in_buf_size - ((char*)inout_array - (char*)inout_buf),
            inout_handles_buffer);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR:
      union_buf = ((union MojomPointer*)inout_buf)->ptr;
      encode_pointer(inout_buf, in_buf_size);
      // Fall through
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION: {
      struct MojomUnionLayout* u_data = union_buf;
      if (!in_nullable || (u_data != NULL && u_data->size != 0))
        MojomUnion_EncodePointersAndHandles(
            (const struct MojomTypeDescriptorUnion*)in_type_desc,
            inout_buf,
            in_buf_size,
            inout_handles_buffer);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
      encode_handle(in_nullable, (MojoHandle*)inout_buf, inout_handles_buffer);
      break;
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE: {
      struct MojomInterfaceData* interface = inout_buf;
      encode_handle(in_nullable, &interface->handle, inout_handles_buffer);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      // We shouldn't ever end up here.
      assert(false);
      break;
  }
}

void MojomType_DispatchDecodePointersAndHandles(
    enum MojomTypeDescriptorType in_elem_type,
    const void* in_type_desc,
    bool in_nullable,
    void* inout_buf,
    uint32_t in_buf_size,
    MojoHandle* inout_handles,
    uint32_t in_num_handles) {
  assert(inout_buf);

  void* union_buf = inout_buf;
  switch (in_elem_type) {
    case MOJOM_TYPE_DESCRIPTOR_TYPE_STRUCT_PTR: {
      decode_pointer(inout_buf);
      struct MojomStructHeader* inout_struct =
          ((union MojomPointer*)inout_buf)->ptr;
      assert(inout_struct == NULL ||
             (char*)inout_struct < ((char*)inout_buf) + in_buf_size);
      if (!in_nullable || inout_struct != NULL)
        MojomStruct_DecodePointersAndHandles(
            (const struct MojomTypeDescriptorStruct*)in_type_desc,
            inout_struct,
            in_buf_size - ((char*)inout_struct - (char*)inout_buf),
            inout_handles,
            in_num_handles);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_ARRAY_PTR: {
      decode_pointer(inout_buf);
      struct MojomArrayHeader* inout_array =
                ((union MojomPointer*)inout_buf)->ptr;
      assert(inout_array == NULL ||
             (char*)inout_array < ((char*)inout_buf) + in_buf_size);
      if (!in_nullable || inout_array != NULL)
        MojomArray_DecodePointersAndHandles(
            (const struct MojomTypeDescriptorArray*)in_type_desc,
            inout_array,
            in_buf_size - ((char*)inout_array - (char*)inout_buf),
            inout_handles,
            in_num_handles);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION_PTR:
      decode_pointer(inout_buf);
      union_buf = ((union MojomPointer*)inout_buf)->ptr;
      assert(union_buf == NULL ||
             (char*)union_buf < ((char*)inout_buf) + in_buf_size);
      // Fall through
    case MOJOM_TYPE_DESCRIPTOR_TYPE_UNION: {
      struct MojomUnionLayout* u_data = union_buf;
      if (!in_nullable || (u_data != NULL && u_data->size != 0))
        MojomUnion_DecodePointersAndHandles(
            (const struct MojomTypeDescriptorUnion*)in_type_desc,
            inout_buf,
            in_buf_size,
            inout_handles,
            in_num_handles);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_HANDLE:
      decode_handle((MojoHandle*)inout_buf, inout_handles,
                    in_num_handles);
      break;
    case MOJOM_TYPE_DESCRIPTOR_TYPE_INTERFACE: {
      struct MojomInterfaceData* interface = inout_buf;
      decode_handle(&interface->handle, inout_handles,
                    in_num_handles);
      break;
    }
    case MOJOM_TYPE_DESCRIPTOR_TYPE_POD:
      // We shouldn't ever end up here.
      assert(false);
      break;
  }
}
