#include "quickmem.h"
#include "memio.h"
#include <quickjs.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <cstdint>
#include <vector>
#include <new>
#include <sstream>
#include <fstream>

// Global QuickJS state
static JSRuntime* g_rt = nullptr;
static JSContext* g_ctx = nullptr;
static QuickMemContext* g_qm_ctx = nullptr;
JSClassID js_native_pointer_class_id = 0;

/**
 * Finalizer for NativePointer objects.
 * Frees the opaque NativePointerData.
 */
static void js_native_pointer_finalizer(JSRuntime* rt, JSValue val) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(val, js_native_pointer_class_id));
    if (data) {
        delete data;
    }
}

/**
 * NativePointer.prototype.toString() - returns hex string like "0x7ffd0000"
 */
static JSValue js_native_pointer_toString(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%" PRIxPTR, static_cast<uintptr_t>(data->address));
    return JS_NewString(ctx, buf);
}

// ============== Numeric Read Methods ==============

static JSValue js_readS8(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    int8_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewInt32(ctx, val);
}

static JSValue js_readU8(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    uint8_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewInt32(ctx, val);
}

static JSValue js_readS16(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    int16_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewInt32(ctx, val);
}

static JSValue js_readU16(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    uint16_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewInt32(ctx, val);
}

static JSValue js_readS32(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    int32_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewInt32(ctx, val);
}

static JSValue js_readU32(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    uint32_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewInt64(ctx, val);  // Use Int64 to hold 32-bit unsigned
}

static JSValue js_readS64(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    int64_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewInt64(ctx, val);
}

static JSValue js_readU64(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    uint64_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewInt64(ctx, static_cast<int64_t>(val));
}

static JSValue js_readFloat(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    float val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewFloat64(ctx, val);
}

static JSValue js_readDouble(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    double val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    return JS_NewFloat64(ctx, val);
}

static JSValue js_readPointer(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(
        JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    uintptr_t val;
    if (pm_read(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    
    // Create new NativePointer with read address
    NativePointerData* new_data = new (std::nothrow) NativePointerData;
    if (!new_data) {
        return JS_ThrowOutOfMemory(ctx);
    }
    new_data->address = val;
    new_data->pid = data->pid;
    
    JSValue obj = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(obj)) {
        delete new_data;
        return JS_EXCEPTION;
    }
    
    JS_SetOpaque(obj, new_data);
    return obj;
}

// ============== Arithmetic Operator Methods ==============

/**
 * NativePointer.prototype.add(val) - returns new NativePointer with address + val
 */
static JSValue js_native_pointer_add(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "add requires an argument");
    }
    
    int64_t val;
    if (JS_ToInt64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    // Allocate new NativePointerData
    NativePointerData* new_data = new (std::nothrow) NativePointerData;
    if (!new_data) {
        return JS_ThrowOutOfMemory(ctx);
    }
    new_data->address = data->address + static_cast<uintptr_t>(val);
    new_data->pid = data->pid;
    
    JSValue obj = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(obj)) {
        delete new_data;
        return JS_EXCEPTION;
    }
    
    JS_SetOpaque(obj, new_data);
    return obj;
}

/**
 * NativePointer.prototype.sub(val) - returns new NativePointer with address - val
 */
static JSValue js_native_pointer_sub(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "sub requires an argument");
    }
    
    int64_t val;
    if (JS_ToInt64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    NativePointerData* new_data = new (std::nothrow) NativePointerData;
    if (!new_data) {
        return JS_ThrowOutOfMemory(ctx);
    }
    new_data->address = data->address - static_cast<uintptr_t>(val);
    new_data->pid = data->pid;
    
    JSValue obj = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(obj)) {
        delete new_data;
        return JS_EXCEPTION;
    }
    
    JS_SetOpaque(obj, new_data);
    return obj;
}

/**
 * NativePointer.prototype.xor(val) - returns new NativePointer with address ^ val
 */
static JSValue js_native_pointer_xor(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "xor requires an argument");
    }
    
    int64_t val;
    if (JS_ToInt64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    NativePointerData* new_data = new (std::nothrow) NativePointerData;
    if (!new_data) {
        return JS_ThrowOutOfMemory(ctx);
    }
    new_data->address = data->address ^ static_cast<uintptr_t>(val);
    new_data->pid = data->pid;
    
    JSValue obj = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(obj)) {
        delete new_data;
        return JS_EXCEPTION;
    }
    
    JS_SetOpaque(obj, new_data);
    return obj;
}

/**
 * NativePointer.prototype.shr(val) - returns new NativePointer with address >> val
 */
static JSValue js_native_pointer_shr(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "shr requires an argument");
    }
    
    int64_t val;
    if (JS_ToInt64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    NativePointerData* new_data = new (std::nothrow) NativePointerData;
    if (!new_data) {
        return JS_ThrowOutOfMemory(ctx);
    }
    new_data->address = data->address >> static_cast<int>(val);
    new_data->pid = data->pid;
    
    JSValue obj = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(obj)) {
        delete new_data;
        return JS_EXCEPTION;
    }
    
    JS_SetOpaque(obj, new_data);
    return obj;
}

/**
 * NativePointer.prototype.shl(val) - returns new NativePointer with address << val
 */
static JSValue js_native_pointer_shl(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "shl requires an argument");
    }
    
    int64_t val;
    if (JS_ToInt64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    NativePointerData* new_data = new (std::nothrow) NativePointerData;
    if (!new_data) {
        return JS_ThrowOutOfMemory(ctx);
    }
    new_data->address = data->address << static_cast<int>(val);
    new_data->pid = data->pid;
    
    JSValue obj = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(obj)) {
        delete new_data;
        return JS_EXCEPTION;
    }
    
    JS_SetOpaque(obj, new_data);
    return obj;
}

// ============== Numeric Write Methods ==============

static JSValue js_writeS8(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeS8 requires an argument");
    }
    
    int32_t val;
    if (JS_ToInt32(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    int8_t native_val = static_cast<int8_t>(val);
    if (pm_write(data->pid, &native_val, sizeof(native_val), data->address) != sizeof(native_val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeU8(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeU8 requires an argument");
    }
    
    int32_t val;
    if (JS_ToInt32(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    uint8_t native_val = static_cast<uint8_t>(val);
    if (pm_write(data->pid, &native_val, sizeof(native_val), data->address) != sizeof(native_val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeS16(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeS16 requires an argument");
    }
    
    int32_t val;
    if (JS_ToInt32(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    int16_t native_val = static_cast<int16_t>(val);
    if (pm_write(data->pid, &native_val, sizeof(native_val), data->address) != sizeof(native_val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeU16(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeU16 requires an argument");
    }
    
    int32_t val;
    if (JS_ToInt32(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    uint16_t native_val = static_cast<uint16_t>(val);
    if (pm_write(data->pid, &native_val, sizeof(native_val), data->address) != sizeof(native_val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeS32(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeS32 requires an argument");
    }
    
    int32_t val;
    if (JS_ToInt32(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    if (pm_write(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeU32(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeU32 requires an argument");
    }
    
    int64_t val;
    if (JS_ToInt64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    uint32_t native_val = static_cast<uint32_t>(val);
    if (pm_write(data->pid, &native_val, sizeof(native_val), data->address) != sizeof(native_val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeS64(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeS64 requires an argument");
    }
    
    int64_t val;
    if (JS_ToInt64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    if (pm_write(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeU64(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeU64 requires an argument");
    }
    
    int64_t val;
    if (JS_ToInt64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    uint64_t native_val = static_cast<uint64_t>(val);
    if (pm_write(data->pid, &native_val, sizeof(native_val), data->address) != sizeof(native_val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeFloat(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeFloat requires an argument");
    }
    
    double val;
    if (JS_ToFloat64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    float native_val = static_cast<float>(val);
    if (pm_write(data->pid, &native_val, sizeof(native_val), data->address) != sizeof(native_val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

static JSValue js_writeDouble(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeDouble requires an argument");
    }
    
    double val;
    if (JS_ToFloat64(ctx, &val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    if (pm_write(data->pid, &val, sizeof(val), data->address) != sizeof(val)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    return JS_UNDEFINED;
}

// ============== Byte Array Methods ==============

static JSValue js_readByteArray(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "readByteArray requires length argument");
    }
    
    int32_t length;
    if (JS_ToInt32(ctx, &length, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    if (length <= 0) {
        return JS_ThrowTypeError(ctx, "readByteArray: length must be positive");
    }
    if (length > 1024 * 1024) {
        return JS_ThrowTypeError(ctx, "readByteArray: length exceeds 1MB limit");
    }
    
    // Allocate buffer
    std::vector<uint8_t> buf(length);
    ssize_t bytes_read = pm_read(data->pid, buf.data(), length, data->address);
    if (bytes_read != length) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    
    return JS_NewArrayBufferCopy(ctx, buf.data(), length);
}

static JSValue js_writeByteArray(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeByteArray requires bytes argument");
    }
    
    size_t buf_len;
    uint8_t* buf = nullptr;
    
    // Try ArrayBuffer first
    if (JS_IsArrayBuffer(argv[0])) {
        buf = JS_GetArrayBuffer(ctx, &buf_len, argv[0]);
        if (!buf) {
            return JS_ThrowTypeError(ctx, "Failed to get ArrayBuffer");
        }
    }
    // Try TypedArray (Uint8Array) - use JS_GetUint8Array for direct access
    else {
        buf = JS_GetUint8Array(ctx, &buf_len, argv[0]);
        if (!buf) {
            return JS_ThrowTypeError(ctx, "writeByteArray requires ArrayBuffer or Uint8Array");
        }
    }
    
    // Cap at 1MB
    if (buf_len > 1024 * 1024) {
        return JS_ThrowTypeError(ctx, "writeByteArray: data exceeds 1MB limit");
    }
    
    if (pm_write(data->pid, buf, buf_len, data->address) != static_cast<ssize_t>(buf_len)) {
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    
    return JS_UNDEFINED;
}

// ============== String Methods ==============

static JSValue js_readUtf8String(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    // Read up to 2048 bytes for null-terminated UTF-8 string
    char buf[2048];
    ssize_t bytes_read = pm_read(data->pid, buf, sizeof(buf), data->address);
    if (bytes_read <= 0) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    
    // Find null terminator
    char* null_pos = static_cast<char*>(std::memchr(buf, '\0', bytes_read));
    if (!null_pos) {
        return JS_ThrowTypeError(ctx, "readUtf8String: no null terminator found within 2048 bytes");
    }
    
    return JS_NewString(ctx, buf);
}

static JSValue js_writeUtf8String(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "writeUtf8String requires string argument");
    }
    
    if (!JS_IsString(argv[0])) {
        return JS_ThrowTypeError(ctx, "writeUtf8String requires a string argument");
    }
    
    size_t len;
    const char* str = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!str) {
        return JS_EXCEPTION;
    }
    
    // Write string + null terminator (capped at 2048)
    size_t write_len = len + 1;
    if (write_len > 2048) {
        write_len = 2048;
    }
    
    if (pm_write(data->pid, str, write_len, data->address) != static_cast<ssize_t>(write_len)) {
        JS_FreeCString(ctx, str);
        return JS_ThrowTypeError(ctx, "Failed to write memory: %s", pm_error_str());
    }
    
    JS_FreeCString(ctx, str);
    return JS_UNDEFINED;
}

static JSValue js_readCString(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    NativePointerData* data = static_cast<NativePointerData*>(JS_GetOpaque(this_val, js_native_pointer_class_id));
    if (!data) {
        return JS_ThrowTypeError(ctx, "Not a NativePointer");
    }
    
    // Read up to 2048 bytes, scan for null terminator
    char buf[2048];
    ssize_t bytes_read = pm_read(data->pid, buf, sizeof(buf), data->address);
    if (bytes_read <= 0) {
        return JS_ThrowTypeError(ctx, "Failed to read memory: %s", pm_error_str());
    }
    
    // Find null terminator
    char* null_pos = static_cast<char*>(std::memchr(buf, '\0', bytes_read));
    if (!null_pos) {
        return JS_ThrowTypeError(ctx, "readCString: no null terminator found within 2048 bytes");
    }
    
    // Return string up to null terminator
    return JS_NewString(ctx, buf);
}

// ============== Pattern Parser ==============

/**
 * Parses a hex pattern string into a vector of bytes.
 * Wildcard value 0x100 means "match any byte".
 * Format: "7f 45 4c 46" or "00 ?? 13 37 ?? 42" (?? is wildcard)
 * Returns false on invalid input.
 */
static bool parse_hex_pattern(const std::string& pattern_str, std::vector<uint16_t>& out) {
    out.clear();
    
    if (pattern_str.empty()) {
        return false;
    }
    
    // Trim leading/trailing whitespace
    size_t start = pattern_str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return false;
    }
    size_t end = pattern_str.find_last_not_of(" \t\r\n");
    std::string trimmed = pattern_str.substr(start, end - start + 1);
    
    // Split by whitespace
    std::istringstream iss(trimmed);
    std::string token;
    
    while (std::getline(iss, token, ' ')) {
        if (token.empty()) {
            continue;
        }
        
        // Check for wildcard
        if (token == "?" || token == "??") {
            out.push_back(0x100);  // Wildcard marker
            continue;
        }
        
        // Try to parse as hex byte
        if (token.length() > 2) {
            return false;
        }
        
        char* endptr = nullptr;
        unsigned long val = std::strtoul(token.c_str(), &endptr, 16);
        if (endptr == token.c_str() || *endptr != '\0') {
            return false;
        }
        
        if (val > 0xFF) {
            return false;
        }
        
        out.push_back(static_cast<uint16_t>(val));
    }
    
    return !out.empty();
}

/**
 * ptr(value) - creates a NativePointer from a number or hex string.
 * Accepts: number (cast to uintptr_t via JS_ToInt64) or hex string ("0xabcd")
 * Rejects: other types with TypeError
 */
static JSValue js_ptr(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "ptr requires an argument");
    }
    
    JSValue arg = argv[0];
    uintptr_t address = 0;
    
    // Check if argument is a number
    if (JS_IsNumber(arg)) {
        int64_t val;
        if (JS_ToInt64(ctx, &val, arg) < 0) {
            return JS_EXCEPTION;
        }
        address = static_cast<uintptr_t>(val);
    }
    // Check if argument is a string
    else if (JS_IsString(arg)) {
        const char* str = JS_ToCString(ctx, arg);
        if (!str) {
            return JS_EXCEPTION;
        }
        
        // Check for "0x" or "0X" prefix for hex
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            char* endptr = nullptr;
            unsigned long long val = std::strtoull(str, &endptr, 16);
            if (endptr == str || *endptr != '\0') {
                JS_FreeCString(ctx, str);
                return JS_ThrowTypeError(ctx, "Invalid hex string");
            }
            address = static_cast<uintptr_t>(val);
        } else {
            JS_FreeCString(ctx, str);
            return JS_ThrowTypeError(ctx, "ptr: non-hex strings not supported, use 0x prefix");
        }
        
        JS_FreeCString(ctx, str);
    }
    // BigInt not supported per requirements
    else if (JS_IsBigInt(arg)) {
        return JS_ThrowTypeError(ctx, "ptr: BigInt not supported");
    }
    // Other types rejected
    else {
        return JS_ThrowTypeError(ctx, "ptr: expected number or hex string");
    }
    
    // Allocate and populate NativePointerData
    NativePointerData* data = new (std::nothrow) NativePointerData;
    if (!data) {
        return JS_ThrowOutOfMemory(ctx);
    }
    data->address = address;
    data->pid = g_qm_ctx->pid;
    
    // Create and return NativePointer object
    JSValue obj = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(obj)) {
        delete data;
        return JS_EXCEPTION;
    }
    
    JS_SetOpaque(obj, data);
    return obj;
}

/**
 * Memory.alloc(size) - allocates local memory in the quickmem process.
 * Signature: function Memory.alloc(size)
 * @param size: number of bytes to allocate (must be > 0 and <= 1MB)
 * @return NativePointer to allocated memory
 */
static JSValue js_memory_alloc(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "Memory.alloc requires size argument");
    }

    int64_t size_val;
    if (JS_ToInt64(ctx, &size_val, argv[0]) < 0) {
        return JS_EXCEPTION;
    }

    // Validate size
    if (size_val <= 0) {
        return JS_ThrowTypeError(ctx, "Memory.alloc: size must be positive");
    }
    if (size_val > 1024 * 1024) {
        return JS_ThrowTypeError(ctx, "Memory.alloc: size exceeds 1MB limit");
    }

    // Allocate memory
    void* ptr = std::malloc(static_cast<size_t>(size_val));
    if (!ptr) {
        return JS_ThrowTypeError(ctx, "Memory.alloc: allocation failed");
    }

    // Zero-initialize the memory
    std::memset(ptr, 0, static_cast<size_t>(size_val));

    // Create NativePointerData with address and local pid
    NativePointerData* data = new (std::nothrow) NativePointerData;
    if (!data) {
        std::free(ptr);
        return JS_ThrowOutOfMemory(ctx);
    }
    data->address = reinterpret_cast<uintptr_t>(ptr);
    data->pid = getpid();

    // Create and return NativePointer object
    JSValue obj = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(obj)) {
        std::free(ptr);
        delete data;
        return JS_EXCEPTION;
    }

    JS_SetOpaque(obj, data);
    return obj;
}

// ============== Memory.scanSync ==============

/**
 * Memory.scanSync(address, size, pattern) - scans memory for a hex pattern.
 * 
 * @param address: NativePointer or number - starting address to scan
 * @param size: number - size of memory region to scan
 * @param pattern: string - hex pattern like "7f 45 4c 46" or "00 ?? 13 37 ?? 42"
 * @return array of {address: NativePointer, size: number}
 */
static JSValue js_memory_scanSync(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    if (argc < 3) {
        return JS_ThrowTypeError(ctx, "Memory.scanSync requires 3 arguments: address, size, pattern");
    }
    
    // Parse address (NativePointer or number)
    uintptr_t start_addr;
    NativePointerData* addr_data = static_cast<NativePointerData*>(
        JS_GetOpaque(argv[0], js_native_pointer_class_id));
    if (addr_data) {
        start_addr = addr_data->address;
    } else {
        int64_t addr_val;
        if (JS_ToInt64(ctx, &addr_val, argv[0]) < 0) {
            return JS_EXCEPTION;
        }
        start_addr = static_cast<uintptr_t>(addr_val);
    }
    
    // Parse size
    int64_t size_val;
    if (JS_ToInt64(ctx, &size_val, argv[1]) < 0) {
        return JS_EXCEPTION;
    }
    if (size_val <= 0 || size_val > 1024 * 1024 * 1024) {
        return JS_ThrowTypeError(ctx, "Memory.scanSync: invalid size");
    }
    size_t scan_size = static_cast<size_t>(size_val);
    
    // Parse pattern string
    const char* pattern_cstr = JS_ToCString(ctx, argv[2]);
    if (!pattern_cstr) {
        return JS_EXCEPTION;
    }
    std::vector<uint16_t> pattern;
    bool pattern_ok = parse_hex_pattern(pattern_cstr, pattern);
    JS_FreeCString(ctx, pattern_cstr);
    
    if (!pattern_ok || pattern.empty()) {
        return JS_ThrowTypeError(ctx, "Memory.scanSync: invalid pattern");
    }
    
    const size_t pattern_len = pattern.size();
    const size_t CHUNK_SIZE = 64 * 1024;  // 64KB chunks
    
    // Result array
    JSValue results = JS_NewArray(ctx);
    int result_idx = 0;
    
    // Read and scan in chunks
    std::vector<uint8_t> buffer(CHUNK_SIZE + pattern_len - 1);
    
    for (size_t offset = 0; offset < scan_size; offset += CHUNK_SIZE) {
        size_t chunk_len = std::min(CHUNK_SIZE, scan_size - offset);
        size_t read_len = chunk_len;
        
        // For overlap, read extra bytes if not the last chunk
        if (offset + chunk_len < scan_size) {
            read_len = chunk_len + pattern_len - 1;
        }
        
        read_len = std::min(read_len, scan_size - offset);
        
        ssize_t n = pm_read(g_qm_ctx->pid, buffer.data(), read_len, start_addr + offset);
        if (n <= 0) continue;  // Skip unreadable regions
        
        size_t actual_read = static_cast<size_t>(n);
        
        // Scan this chunk
        for (size_t i = 0; i + pattern_len <= actual_read; i++) {
            bool match = true;
            for (size_t j = 0; j < pattern_len; j++) {
                if (pattern[j] != 0x100 && buffer[i + j] != static_cast<uint8_t>(pattern[j])) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                // Don't report matches from overlap region if this isn't the first chunk
                if (offset > 0 && i < pattern_len - 1) {
                    continue;
                }
                
                uintptr_t match_addr = start_addr + offset + i;
                
                // Create match object
                JSValue match_obj = JS_NewObject(ctx);
                
                // address as NativePointer
                NativePointerData* match_data = new (std::nothrow) NativePointerData;
                if (match_data) {
                    match_data->address = match_addr;
                    match_data->pid = g_qm_ctx->pid;
                    JSValue addr_ptr = JS_NewObjectClass(ctx, js_native_pointer_class_id);
                    JS_SetOpaque(addr_ptr, match_data);
                    JS_SetPropertyStr(ctx, match_obj, "address", addr_ptr);
                }
                
                JS_SetPropertyStr(ctx, match_obj, "size", JS_NewInt64(ctx, static_cast<int64_t>(pattern_len)));
                
                JS_SetPropertyUint32(ctx, results, result_idx++, match_obj);
            }
        }
    }
    
    return results;
}

// ============== hexdump ==============

/**
 * hexdump(target, options) - produces a hexdump string similar to Frida's API.
 * 
 * @param target: NativePointer or ArrayBuffer to dump
 * @param options: optional object {
 *     address: NativePointer - base address for display (default: target address or 0)
 *     offset: number - byte offset to start from (default: 0)
 *     length: number - how many bytes to dump (default: all available)
 *     header: boolean - include header row (default: true)
 * }
 * @return formatted hexdump string
 */
static JSValue js_hexdump(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "hexdump requires a target argument");
    }
    
    // Determine target type and extract data
    uint8_t* data_ptr = nullptr;
    size_t data_len = 0;
    uintptr_t base_address = 0;
    pid_t pid = 0;
    
    // Check if NativePointer
    if (JS_GetClassID(argv[0]) == js_native_pointer_class_id) {
        NativePointerData* np_data = static_cast<NativePointerData*>(
            JS_GetOpaque(argv[0], js_native_pointer_class_id));
        if (!np_data) {
            return JS_ThrowTypeError(ctx, "hexdump: invalid NativePointer");
        }
        
        pid = np_data->pid;
        base_address = np_data->address;
        
        // For NativePointer without length, we need a default length
        // We'll use the options.length if provided, otherwise read 256 bytes by default
        // But first, check if length is provided in options
    }
    // Check if ArrayBuffer
    else if (JS_IsArrayBuffer(argv[0])) {
        size_t buf_len = 0;
        data_ptr = JS_GetArrayBuffer(ctx, &buf_len, argv[0]);
        if (!data_ptr) {
            return JS_ThrowTypeError(ctx, "hexdump: failed to get ArrayBuffer data");
        }
        data_len = buf_len;
        // ArrayBuffer has no associated address, base_address stays 0
    }
    else {
        return JS_ThrowTypeError(ctx, "hexdump: target must be a NativePointer or ArrayBuffer");
    }
    
    // Parse options object (2nd argument)
    uintptr_t display_address = base_address;
    int64_t offset = 0;
    int64_t length = -1;  // -1 means "all available"
    int header = 1;       // default: include header
    
    if (argc >= 2 && JS_IsObject(argv[1])) {
        JSValue opts_obj = argv[1];
        
        // address option
        JSValue addr_prop = JS_GetPropertyStr(ctx, opts_obj, "address");
        if (!JS_IsUndefined(addr_prop) && !JS_IsNull(addr_prop)) {
            if (JS_GetClassID(addr_prop) == js_native_pointer_class_id) {
                NativePointerData* addr_data = static_cast<NativePointerData*>(
                    JS_GetOpaque(addr_prop, js_native_pointer_class_id));
                if (addr_data) {
                    display_address = addr_data->address;
                }
            } else {
                // If it's a number, use that as the display address
                int64_t addr_val = 0;
                if (JS_ToInt64(ctx, &addr_val, addr_prop) == 0) {
                    display_address = static_cast<uintptr_t>(addr_val);
                }
            }
        }
        JS_FreeValue(ctx, addr_prop);
        
        // offset option
        JSValue offset_prop = JS_GetPropertyStr(ctx, opts_obj, "offset");
        if (!JS_IsUndefined(offset_prop) && !JS_IsNull(offset_prop)) {
            int64_t offset_val = 0;
            if (JS_ToInt64(ctx, &offset_val, offset_prop) == 0) {
                offset = offset_val;
            }
        }
        JS_FreeValue(ctx, offset_prop);
        
        // length option
        JSValue len_prop = JS_GetPropertyStr(ctx, opts_obj, "length");
        if (!JS_IsUndefined(len_prop) && !JS_IsNull(len_prop)) {
            int64_t len_val = 0;
            if (JS_ToInt64(ctx, &len_val, len_prop) == 0) {
                length = len_val;
            }
        }
        JS_FreeValue(ctx, len_prop);
        
        // header option
        JSValue header_prop = JS_GetPropertyStr(ctx, opts_obj, "header");
        if (!JS_IsUndefined(header_prop) && !JS_IsNull(header_prop)) {
            header = JS_ToBool(ctx, header_prop);
        }
        JS_FreeValue(ctx, header_prop);
    }
    
    // For NativePointer: we need to read the actual bytes
    std::vector<uint8_t> buffer;
    if (JS_GetClassID(argv[0]) == js_native_pointer_class_id) {
        // Determine length for NativePointer
        // If length is -1 (not specified), use 256 as default
        if (length < 0) {
            length = 256;
        }
        
        // Clamp length
        if (length <= 0) {
            return JS_ThrowTypeError(ctx, "hexdump: length must be positive");
        }
        if (length > 1024 * 1024) {
            length = 1024 * 1024;
        }
        
        // Read memory from target
        buffer.resize(static_cast<size_t>(length));
        ssize_t bytes_read = pm_read(pid, buffer.data(), static_cast<size_t>(length), base_address + static_cast<uintptr_t>(offset));
        if (bytes_read < 0) {
            return JS_ThrowTypeError(ctx, "hexdump: failed to read memory: %s", pm_error_str());
        }
        buffer.resize(static_cast<size_t>(bytes_read));
        data_ptr = buffer.data();
        data_len = buffer.size();
        
        // Adjust display_address to account for offset
        display_address += static_cast<uintptr_t>(offset);
    } else {
        // ArrayBuffer: use data from JS buffer
        // Clamp offset
        if (offset < 0) {
            offset = 0;
        }
        if (static_cast<size_t>(offset) >= data_len) {
            return JS_NewString(ctx, "");
        }
        
        // Adjust pointers for offset
        data_ptr += static_cast<size_t>(offset);
        data_len -= static_cast<size_t>(offset);
        
        // Clamp length
        if (length >= 0 && static_cast<size_t>(length) < data_len) {
            data_len = static_cast<size_t>(length);
        }
        
        // Adjust display_address for offset
        display_address += static_cast<uintptr_t>(offset);
    }
    
    // Build the hexdump string
    std::ostringstream out;
    
    const size_t BYTES_PER_LINE = 16;
    size_t pos = 0;
    
    if (header) {
        // Frida-style header
        out << "Address           Hex                                          ASCII\n";
    }
    
    while (pos < data_len) {
        size_t line_len = (data_len - pos < BYTES_PER_LINE) ? (data_len - pos) : BYTES_PER_LINE;
        
        // Address
        char addr_buf[32];
        std::snprintf(addr_buf, sizeof(addr_buf), "%08" PRIxPTR, static_cast<uintptr_t>(display_address + pos));
        out << addr_buf << "  ";
        
        // Hex bytes
        for (size_t i = 0; i < BYTES_PER_LINE; i++) {
            if (i < line_len) {
                char byte_buf[8];
                std::snprintf(byte_buf, sizeof(byte_buf), "%02x", data_ptr[pos + i]);
                out << byte_buf;
            } else {
                out << "  ";
            }
            if (i == 7) {
                out << " ";  // Extra space in middle
            }
            if (i < BYTES_PER_LINE - 1) {
                out << " ";
            }
        }
        
        out << " ";
        
        // ASCII representation
        for (size_t i = 0; i < line_len; i++) {
            uint8_t c = data_ptr[pos + i];
            if (c >= 32 && c < 127) {
                out << static_cast<char>(c);
            } else {
                out << ".";
            }
        }
        
        out << "\n";
        pos += line_len;
    }
    
    return JS_NewString(ctx, out.str().c_str());
}

/**
 * console.log implementation - prints arguments to stdout
 * Signature: function console.log(...args)
 */
// ============== Process.findModuleByName ==============

struct ModuleInfo {
    uintptr_t base;
    size_t size;
    std::string name;
    std::string path;
};

/**
 * Parses /proc/<pid>/maps to find a shared library by name.
 * Returns true if found, fills in ModuleInfo.
 * Includes adjacent anonymous mappings (like [anon:.bss]) in the size.
 */
static bool find_module_by_name(pid_t pid, const std::string& name, ModuleInfo& out) {
    std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream file(maps_path);
    if (!file.is_open()) {
        return false;
    }
    
    struct Entry {
        uintptr_t start;
        uintptr_t end;
        std::string path;
    };
    std::vector<Entry> entries;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string addr_range, perms, offset, dev, inode;
        iss >> addr_range >> perms >> offset >> dev >> inode;
        
        std::string path;
        std::getline(iss, path);
        size_t start_pos = path.find_first_not_of(" \t");
        if (start_pos != std::string::npos) {
            path = path.substr(start_pos);
        }
        
        size_t dash = addr_range.find('-');
        if (dash == std::string::npos) continue;
        
        uintptr_t s = std::stoull(addr_range.substr(0, dash), nullptr, 16);
        uintptr_t e = std::stoull(addr_range.substr(dash + 1), nullptr, 16);
        
        entries.push_back({s, e, path});
    }
    
    // Find the first entry matching the module name
    int first_idx = -1;
    std::string found_path;
    
    for (int i = 0; i < (int)entries.size(); i++) {
        if (entries[i].path.find(name) != std::string::npos) {
            first_idx = i;
            found_path = entries[i].path;
            break;
        }
    }
    
    if (first_idx == -1) {
        return false;
    }
    
    // Find the last contiguous entry that belongs to this module.
    // A contiguous entry is one that is either:
    // 1. Has the module name in its path
    // 2. Is anonymous and immediately adjacent to the previous entry
    int last_idx = first_idx;
    
    for (int i = first_idx + 1; i < (int)entries.size(); i++) {
        bool has_module_name = (entries[i].path.find(name) != std::string::npos);
        bool is_anonymous = entries[i].path.empty() || 
                            entries[i].path.find("[anon:") == 0;
        bool is_adjacent = (entries[i].start == entries[last_idx].end);
        
        if (has_module_name) {
            last_idx = i;
        } else if (is_anonymous && is_adjacent) {
            last_idx = i;
        } else {
            break;
        }
    }
    
    // Also check backward for anonymous regions before the first match
    int start_idx = first_idx;
    for (int i = first_idx - 1; i >= 0; i--) {
        bool is_anonymous = entries[i].path.empty() || 
                            entries[i].path.find("[anon:") == 0;
        bool is_adjacent = (entries[i].end == entries[start_idx].start);
        
        if (is_anonymous && is_adjacent) {
            start_idx = i;
        } else {
            break;
        }
    }
    
    out.base = entries[start_idx].start;
    out.size = entries[last_idx].end - entries[start_idx].start;
    out.name = name;
    out.path = found_path;
    return true;
}

/**
 * Process.findModuleByName(name) - finds a shared library by name.
 * Returns: {base: NativePointer, size: number, name: string, path: string} or null
 */
static JSValue js_process_findModuleByName(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "Process.findModuleByName requires a name argument");
    }
    
    const char* name_cstr = JS_ToCString(ctx, argv[0]);
    if (!name_cstr) {
        return JS_EXCEPTION;
    }
    
    ModuleInfo info;
    bool found = find_module_by_name(g_qm_ctx->pid, name_cstr, info);
    JS_FreeCString(ctx, name_cstr);
    
    if (!found) {
        return JS_NULL;
    }
    
    JSValue obj = JS_NewObject(ctx);
    if (JS_IsException(obj)) {
        return JS_EXCEPTION;
    }
    
    // base as NativePointer
    NativePointerData* data = new (std::nothrow) NativePointerData;
    if (!data) {
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    data->address = info.base;
    data->pid = g_qm_ctx->pid;
    JSValue base_ptr = JS_NewObjectClass(ctx, js_native_pointer_class_id);
    if (JS_IsException(base_ptr)) {
        delete data;
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }
    JS_SetOpaque(base_ptr, data);
    JS_SetPropertyStr(ctx, obj, "base", base_ptr);
    // Do NOT free base_ptr - lives as property value
    
    JS_SetPropertyStr(ctx, obj, "size", JS_NewInt64(ctx, static_cast<int64_t>(info.size)));
    JS_SetPropertyStr(ctx, obj, "name", JS_NewString(ctx, info.name.c_str()));
    JS_SetPropertyStr(ctx, obj, "path", JS_NewString(ctx, info.path.c_str()));
    
    return obj;
}

static JSValue js_console_log(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            std::putchar(' ');
        }
        
        const char* str = JS_ToCString(ctx, argv[i]);
        if (str) {
            std::fputs(str, stdout);
            JS_FreeCString(ctx, str);
        } else {
            // Handle non-string values (undefined, null, objects, etc.)
            JSValue disp = JS_ToString(ctx, argv[i]);
            if (!JS_IsException(disp)) {
                const char* disp_str = JS_ToCString(ctx, disp);
                if (disp_str) {
                    std::fputs(disp_str, stdout);
                    JS_FreeCString(ctx, disp_str);
                }
                JS_FreeValue(ctx, disp);
            }
        }
    }
    std::putchar('\n');
    
    return JS_UNDEFINED;
}

int quickjs_init(pid_t pid) {
    // Create JSRuntime
    g_rt = JS_NewRuntime();
    if (!g_rt) {
        std::fprintf(stderr, "QuickJS: Failed to create runtime (OOM?)\n");
        std::exit(1);
    }
    
    // Set memory limit (optional, but good practice)
    JS_SetMemoryLimit(g_rt, 128 * 1024 * 1024); // 128MB limit
    
    // Create JSContext
    g_ctx = JS_NewContext(g_rt);
    if (!g_ctx) {
        std::fprintf(stderr, "QuickJS: Failed to create context (OOM?)\n");
        JS_FreeRuntime(g_rt);
        g_rt = nullptr;
        std::exit(1);
    }
    
    // Allocate and populate our context struct
    g_qm_ctx = new (std::nothrow) QuickMemContext;
    if (!g_qm_ctx) {
        std::fprintf(stderr, "QuickJS: Failed to allocate QuickMemContext (OOM)\n");
        JS_FreeContext(g_ctx);
        JS_FreeRuntime(g_rt);
        g_ctx = nullptr;
        g_rt = nullptr;
        std::exit(1);
    }
    g_qm_ctx->pid = pid;
    
    // Set context opaque to our struct
    JS_SetContextOpaque(g_ctx, g_qm_ctx);
    
    // Register global console.log function
    JSValue global_obj = JS_GetGlobalObject(g_ctx);
    JSValue console_obj = JS_NewObject(g_ctx);
    
    // Create console.log as a method on console object
    // NOTE: JS_SetPropertyStr does NOT increment refcount in QuickJS-ng v0.14.0
    // So we do NOT free after setting (objects stay alive as property values)
    JS_SetPropertyStr(g_ctx, console_obj, "log", 
                      JS_NewCFunction(g_ctx, js_console_log, "log", 1));
    
    // Set console as global property
    JS_SetPropertyStr(g_ctx, global_obj, "console", console_obj);
    // NO JS_FreeValue here - console_obj lives as property value
    
    // Register global Memory object
    JSValue memory_obj = JS_NewObject(g_ctx);
    JS_SetPropertyStr(g_ctx, memory_obj, "alloc", 
                      JS_NewCFunction(g_ctx, js_memory_alloc, "alloc", 1));
    JS_SetPropertyStr(g_ctx, memory_obj, "scanSync", 
                      JS_NewCFunction(g_ctx, js_memory_scanSync, "scanSync", 3));
    JS_SetPropertyStr(g_ctx, global_obj, "Memory", memory_obj);
    // NO JS_FreeValue here - memory_obj lives as property value
    
    // Register NativePointer class
    JS_NewClassID(g_rt, &js_native_pointer_class_id);
    JSClassDef class_def = {
        "NativePointer",
        js_native_pointer_finalizer,
        nullptr,  // gc_mark - not needed since no JS values in opaque
        nullptr,  // call - not needed, constructor only
        nullptr   // exotic - null means normal object
    };
    JS_NewClass(g_rt, js_native_pointer_class_id, &class_def);
    
    // Create NativePointer prototype with all methods
    // NOTE: Do NOT free individual function references - they live as property values
    JSValue np_proto = JS_NewObject(g_ctx);
    
    // toString
    JS_SetPropertyStr(g_ctx, np_proto, "toString", 
                      JS_NewCFunction(g_ctx, js_native_pointer_toString, "toString", 0));
    
    // Numeric read methods
    JS_SetPropertyStr(g_ctx, np_proto, "readS8", 
                      JS_NewCFunction(g_ctx, js_readS8, "readS8", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readU8", 
                      JS_NewCFunction(g_ctx, js_readU8, "readU8", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readS16", 
                      JS_NewCFunction(g_ctx, js_readS16, "readS16", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readU16", 
                      JS_NewCFunction(g_ctx, js_readU16, "readU16", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readS32", 
                      JS_NewCFunction(g_ctx, js_readS32, "readS32", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readU32", 
                      JS_NewCFunction(g_ctx, js_readU32, "readU32", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readS64", 
                      JS_NewCFunction(g_ctx, js_readS64, "readS64", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readU64", 
                      JS_NewCFunction(g_ctx, js_readU64, "readU64", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readFloat", 
                      JS_NewCFunction(g_ctx, js_readFloat, "readFloat", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readDouble", 
                      JS_NewCFunction(g_ctx, js_readDouble, "readDouble", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "readPointer", 
                      JS_NewCFunction(g_ctx, js_readPointer, "readPointer", 0));
    
    // Numeric write methods
    JS_SetPropertyStr(g_ctx, np_proto, "writeS8", 
                      JS_NewCFunction(g_ctx, js_writeS8, "writeS8", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeU8", 
                      JS_NewCFunction(g_ctx, js_writeU8, "writeU8", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeS16", 
                      JS_NewCFunction(g_ctx, js_writeS16, "writeS16", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeU16", 
                      JS_NewCFunction(g_ctx, js_writeU16, "writeU16", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeS32", 
                      JS_NewCFunction(g_ctx, js_writeS32, "writeS32", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeU32", 
                      JS_NewCFunction(g_ctx, js_writeU32, "writeU32", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeS64", 
                      JS_NewCFunction(g_ctx, js_writeS64, "writeS64", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeU64", 
                      JS_NewCFunction(g_ctx, js_writeU64, "writeU64", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeFloat", 
                      JS_NewCFunction(g_ctx, js_writeFloat, "writeFloat", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeDouble", 
                      JS_NewCFunction(g_ctx, js_writeDouble, "writeDouble", 1));
    
    // Byte array methods
    JS_SetPropertyStr(g_ctx, np_proto, "readByteArray", 
                      JS_NewCFunction(g_ctx, js_readByteArray, "readByteArray", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "writeByteArray", 
                      JS_NewCFunction(g_ctx, js_writeByteArray, "writeByteArray", 1));
    
    // String methods
    JS_SetPropertyStr(g_ctx, np_proto, "readUtf8String", 
                      JS_NewCFunction(g_ctx, js_readUtf8String, "readUtf8String", 0));
    JS_SetPropertyStr(g_ctx, np_proto, "writeUtf8String", 
                      JS_NewCFunction(g_ctx, js_writeUtf8String, "writeUtf8String", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "readCString", 
                      JS_NewCFunction(g_ctx, js_readCString, "readCString", 0));
    
    // Arithmetic operator methods
    JS_SetPropertyStr(g_ctx, np_proto, "add", 
                      JS_NewCFunction(g_ctx, js_native_pointer_add, "add", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "sub", 
                      JS_NewCFunction(g_ctx, js_native_pointer_sub, "sub", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "xor", 
                      JS_NewCFunction(g_ctx, js_native_pointer_xor, "xor", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "shr", 
                      JS_NewCFunction(g_ctx, js_native_pointer_shr, "shr", 1));
    JS_SetPropertyStr(g_ctx, np_proto, "shl", 
                      JS_NewCFunction(g_ctx, js_native_pointer_shl, "shl", 1));
    
    // Set the prototype on the class so instances can access it
    // NOTE: Do NOT free np_proto - it lives as class prototype
    JS_SetClassProto(g_ctx, js_native_pointer_class_id, np_proto);
    // NO JS_FreeValue here - np_proto lives as class prototype
    
    // Register global ptr function
    JS_SetPropertyStr(g_ctx, global_obj, "ptr", 
                      JS_NewCFunction(g_ctx, js_ptr, "ptr", 1));
    
    // Register global hexdump function
    JS_SetPropertyStr(g_ctx, global_obj, "hexdump",
                      JS_NewCFunction(g_ctx, js_hexdump, "hexdump", 2));

    // Register global Process object
    JSValue process_obj = JS_NewObject(g_ctx);
    JS_SetPropertyStr(g_ctx, process_obj, "findModuleByName",
                      JS_NewCFunction(g_ctx, js_process_findModuleByName, "findModuleByName", 1));
    JS_SetPropertyStr(g_ctx, global_obj, "Process", process_obj);
    // Do NOT free process_obj - lives as property value

    // Only free global_obj - JS_GetGlobalObject creates a reference we need to release
    JS_FreeValue(g_ctx, global_obj);
    
    return 0;
}

void quickjs_cleanup(void) {
    if (g_ctx) {
        JS_FreeContext(g_ctx);
        g_ctx = nullptr;
    }
    
    if (g_rt) {
        JS_FreeRuntime(g_rt);
        g_rt = nullptr;
    }
    
    if (g_qm_ctx) {
        delete g_qm_ctx;
        g_qm_ctx = nullptr;
    }
}

JSContext* quickjs_ctx(void) {
    return g_ctx;
}
