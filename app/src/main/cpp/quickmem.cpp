#include "quickmem.h"
#include "memio.h"
#include <quickjs.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <cstdint>
#include <vector>
#include <new>

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
 * Memory.alloc stub - throws Error('not implemented')
 * Signature: function Memory.alloc()
 */
static JSValue js_memory_alloc(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    return JS_ThrowTypeError(ctx, "not implemented");
}

/**
 * console.log implementation - prints arguments to stdout
 * Signature: function console.log(...args)
 */
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
