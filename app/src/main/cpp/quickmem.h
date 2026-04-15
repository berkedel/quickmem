#ifndef QUICKMEM_H
#define QUICKMEM_H

#include <quickjs.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/**
 * Context struct passed to QuickJS via JS_SetContextOpaque.
 * Contains the target process PID for memory operations.
 */
typedef struct QuickMemContext {
    pid_t pid;
} QuickMemContext;

/**
 * Opaque data stored in NativePointer JS objects.
 * Contains the native memory address and target PID.
 */
typedef struct NativePointerData {
    uintptr_t address;
    pid_t pid;
} NativePointerData;

/**
 * JSClassID for NativePointer class.
 * Registered during quickjs_init().
 */
extern JSClassID js_native_pointer_class_id;

/**
 * Initialize QuickJS runtime with the given target PID.
 * Must be called before any JS evaluation.
 * On OOM, prints to stderr and calls exit(1).
 *
 * @param pid Target process ID to attach/memory-read
 * @return 0 on success, -1 on failure
 */
int quickjs_init(pid_t pid);

/**
 * Clean up QuickJS runtime and free all associated resources.
 * Safe to call multiple times (idempotent after first call).
 */
void quickjs_cleanup(void);

/**
 * Get the current QuickJS context.
 * Must be called after quickjs_init() and before quickjs_cleanup().
 *
 * @return The JSContext* for the current runtime
 */
JSContext* quickjs_ctx(void);

#endif // QUICKMEM_H
