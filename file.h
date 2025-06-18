/*
  MIT License

  Copyright (c) 2025 Fouzan MD Ishaque (fouzanmdishaque@gmail.com)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/* Utilities for handling files that should have been part of the standard library. */

#ifndef NOVA_FILE_H_INCLUDED_
#define NOVA_FILE_H_INCLUDED_

#include "alloc.h"
#include "errorcodes.h"
#include "stdafx.h"

#if !defined(_WIN32)
#  include <unistd.h>
#  ifndef _POSIX_VERSION
#    warning "No POSIX implementation was found, and the OS is not windows, can not use the file API on this platform"
#    define NOVA_FILE_API_AVAILABLE false
#  endif
#else
#  define NOVA_FILE_API_AVAILABLE true
#endif

#ifndef NOVA_FILE_API_AVAILABLE
#  define NOVA_FILE_API_AVAILABLE true
#endif

NOVA_HEADER_START

typedef enum nv_fs_permission
{
  NV_FS_PERMISSION_EXISTS     = 0,
  NV_FS_PERMISSION_READ_ONLY  = 4,
  NV_FS_PERMISSION_WRITE_ONLY = 2,
  NV_FS_PERMISSION_READ_WRITE = 6,
} nv_fs_permission;

extern bool nv_fs_file_exists(const char* fpath);

/**
 * Returns -1 on invalid input.
 * Windows permissions are available even on unix
 */
extern int nv_fs_perms_to_win_perms(nv_fs_permission perms, bool for_directory);

/**
 * Returns -1 on invalid input or
 * if the platform is windows.
 */
extern int nv_fs_perms_to_unix_perms(nv_fs_permission perms, bool for_directory);

/**
 * If file does not exist, out_size is set to 0.
 */
extern nv_error nv_fs_file_size(const char* fpath, size_t* out_size);

/**
 * If file does not exist, out_atime is set to 0
 */
extern nv_error nv_fs_file_get_access_time(const char* fpath, size_t* out_atime);

/**
 * Get the time that the file was last modified in.
 */
extern nv_error nv_fs_file_get_modified_time(const char* fpath, size_t* out_mtime);

/**
 * Read all of the data of a file into a buffer, allocated with 'alloc'
 * Note that the buffer is NULL terminated and the terminator is not included in buffer_size
 */
extern nv_error nv_fs_file_read_all(const char* fpath, nv_allocator_fn alloc, void* alloc_arg, char** buffer, size_t* buffer_size);

/**
 * If the user has passed in a directory that does not exist, this function will fail
 * and return NV_ERROR_INVALID_OPERATION. You need to create all parent directories before writing.
 */
extern nv_error nv_fs_file_write_all(const char* fpath, const void* data, size_t data_size);

/**
 * Create an empty file.
 * You will need to create all parent directories before calling this function.
 */
extern nv_error nv_fs_file_create(const char* fpath);

/**
 * If the file does not exist, returns NV_ERROR_FILE_NOT_FOUND, though technically not an error.
 * It is invalid to attempt to remove non-empty directories through this function.
 */
extern nv_error nv_fs_file_delete(const char* fpath);

/**
 * Must create parent directories before calling this function.
 */
extern nv_error nv_fs_dir_create(const char* dpath, nv_fs_permission perms);

/**
 * Recursively create all parent directories of the directory path specified.
 * It is invalid to call this with a 'dpath' that references a file, not a directory as this function
 * will create a directory with the name of the file (and overwrite it).
 */
extern nv_error nv_fs_dir_create_recursive(const char* dpath, nv_fs_permission perms);

/**
 * Create the directories for a file, but not the file itself.
 */
extern nv_error nv_fs_dir_create_recursive_for_file(const char* fpath, nv_fs_permission perms);

/**
 * Though confusing, returns NV_ERROR_FILE_NOT_FOUND if the directory does not
 * exist.
 * WARNING: Removes everything from the directory. (Of which we have permission to delete)
 * If a file exists in the directory which we do not have permissions to delete, the file will
 * not be deleted and NV_ERROR_INVALID_OPERATION is returned.
 */
extern nv_error nv_fs_dir_delete_recursive(const char* dpath);

NOVA_HEADER_END

#endif //__NOVA_FILE_H__
