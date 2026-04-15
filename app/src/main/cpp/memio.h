#ifndef MEMIO_H
#define MEMIO_H

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

/**
 * pm_read - Read memory from another process using process_vm_readv
 * @pid: Process ID of the target process
 * @local_buf: Local buffer to store read data
 * @len: Number of bytes to read
 * @remote_addr: Address in the remote process's address space
 * @return: Number of bytes read on success, -1 on failure
 *          If return != len, treat as partial/failure
 */
ssize_t pm_read(pid_t pid, void *local_buf, size_t len, uintptr_t remote_addr);

/**
 * pm_write - Write memory to another process using process_vm_writev
 * @pid: Process ID of the target process
 * @local_buf: Local buffer containing data to write
 * @len: Number of bytes to write
 * @remote_addr: Destination address in the remote process's address space
 * @return: Number of bytes written on success, -1 on failure
 *          If return != len, treat as partial/failure
 */
ssize_t pm_write(pid_t pid, const void *local_buf, size_t len, uintptr_t remote_addr);

/**
 * pm_last_error - Get the errno from the last pm_read or pm_write call
 * @return: The errno value from the most recent operation
 */
int pm_last_error();

/**
 * pm_error_str - Get a descriptive string for the last pm_read or pm_write error
 * @return: Human-readable error string describing the last failure
 */
const char *pm_error_str();

/**
 * pm_error_string - Alias for pm_error_str for convenience
 * @return: Human-readable error string describing the last failure
 */
const char *pm_error_string();

#endif // MEMIO_H
