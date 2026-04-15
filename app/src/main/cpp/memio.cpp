#include "memio.h"

#include <cerrno>
#include <cstring>
#include <sys/uio.h>
#include <unistd.h>

namespace {

// Thread-local storage for the last error encountered
thread_local int g_last_errno = 0;

/**
 * Map errno value to descriptive string
 * Handles: EPERM, ESRCH, EFAULT, EINVAL, and default case
 */
const char *errno_to_string(int err) {
    switch (err) {
        case EPERM:
            return "EPERM: Operation not permitted (insufficient privileges to access process memory)";
        case ESRCH:
            return "ESRCH: No such process (process does not exist or is zombie)";
        case EFAULT:
            return "EFAULT: Bad address (invalid memory address in local_buf or remote_addr)";
        case EINVAL:
            return "EINVAL: Invalid argument (invalid flags, address alignment issue, or other invalid parameter)";
        default:
            return "Unknown error occurred";
    }
}

} // anonymous namespace

ssize_t pm_read(pid_t pid, void *local_buf, size_t len, uintptr_t remote_addr) {
    // Reset errno before syscall
    errno = 0;

    // Setup iovec structures for process_vm_readv
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = local_buf;
    local[0].iov_len = len;

    remote[0].iov_base = reinterpret_cast<void *>(remote_addr);
    remote[0].iov_len = len;

    // Perform the read operation
    ssize_t bytes_read = process_vm_readv(pid, local, 1, remote, 1, 0);

    // Store errno for error reporting
    g_last_errno = errno;

    // Check for partial read - if bytes_read >= 0 but != len, it's a partial success
    // According to spec, we treat return != len as failure
    if (bytes_read != -1 && static_cast<size_t>(bytes_read) != len) {
        g_last_errno = EFAULT;  // Indicate partial read as a fault condition
        return -1;
    }

    return bytes_read;
}

ssize_t pm_write(pid_t pid, const void *local_buf, size_t len, uintptr_t remote_addr) {
    // Reset errno before syscall
    errno = 0;

    // Setup iovec structures for process_vm_writev
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = const_cast<void *>(local_buf);
    local[0].iov_len = len;

    remote[0].iov_base = reinterpret_cast<void *>(remote_addr);
    remote[0].iov_len = len;

    // Perform the write operation
    ssize_t bytes_written = process_vm_writev(pid, local, 1, remote, 1, 0);

    // Store errno for error reporting
    g_last_errno = errno;

    // Check for partial write - treat return != len as failure
    if (bytes_written != -1 && static_cast<size_t>(bytes_written) != len) {
        g_last_errno = EFAULT;  // Indicate partial write as a fault condition
        return -1;
    }

    return bytes_written;
}

int pm_last_error() {
    return g_last_errno;
}

const char *pm_error_str() {
    return errno_to_string(g_last_errno);
}

const char *pm_error_string() {
    return errno_to_string(g_last_errno);
}
