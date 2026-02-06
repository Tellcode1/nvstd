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

#ifndef NV_STD_FILE_H
#define NV_STD_FILE_H

#include "alloc.h"
#include "error.h"
#include "stdafx.h"

#include <stddef.h>

#if !defined(_WIN32)
#  include <unistd.h>
#  ifndef _POSIX_VERSION
#    error "No POSIX implementation was found, and the OS is not windows, can not use the file API on this platform"
#    define NOVA_FILE_API_AVAILABLE false
#  else
#    define NOVA_FILE_API_AVAILABLE true
#  endif
#else
#  define NOVA_FILE_API_AVAILABLE true
#endif

NOVA_HEADER_START

typedef enum nvfs_permission
{
  NVFS_PERMISSION_EXISTS     = 0,
  NVFS_PERMISSION_READ_ONLY  = 1,
  NVFS_PERMISSION_WRITE_ONLY = 2,
  NVFS_PERMISSION_READ_WRITE = 3,
} nvfs_permission;

typedef enum nvfs_type
{
  NVFS_FILE,
  NVFS_DIR,
  NVFS_LINK,
  NVFS_UNKNOWN,
} nvfs_type;

typedef struct nvfs_entry
{
  nvfs_type type;

  /**
   * The relative path to the entry.
   * If unclear, path is relative to the CWD of the program.
   */
  char* relpath;
} nvfs_entry_t;

/**
 * Convert a windows path (\\) to a unix path (/).
 * Disks like C:/ and D:/ can not be converted though.
 * The returned string is malloc'd and must be freed by the user.
 */
char* nvfs_win_to_unix_path(const char* path);

/**
 * Query if an entry exists and is within your permissions or not.
 */
bool nvfs_entry_exists(const char* fpath);

/**
 * Returns -1 on invalid input.
 * Windows permissions are available even on unix
 */
int nvfs_perms_to_win_perms(nvfs_permission perms, bool for_directory);

/**
 * Returns -1 on invalid input or
 * if the platform is windows.
 */
int nvfs_perms_to_unix_perms(nvfs_permission perms, bool for_directory);

/**
 * Get file extension. NULL if none.
 * For multi-extensions like vim.tar.gz, returns the whole .tar.gz extension.
 * string is allocated with nv_zmalloc and must be freed by callee.
 * Doesn't do actual run time checking of the file's actual extension, only tries to guess it from the path.
 * returns NULL on error.
 */
char* nvfs_file_extension(const char* path);

/**
 * Get parent directory of file, may never return NULL.
 * Paths are relative, and are allocated with nv_zmalloc and must be freed by callee.
 */
char* nvfs_file_dir(const char* fpath);

/**
 * Get the type of the entry.
 * Returns NVFS_UNKNOWN if entry does not exist.
 */
nvfs_type nvfs_entry_type(const char* path);

#define nvfs_file_exists(path) (nvfs_entry_type(path) == NVFS_FILE)
#define nvfs_dir_exists(path) (nvfs_entry_type(path) == NVFS_DIR)
#define nvfs_link_exists(path) (nvfs_entry_type(path) == NVFS_LINK)

/**
 * Get if the object at path is a directory or not.
 * Also returns false if it doesn't exist.
 */
bool nvfs_is_dir(const char* path);

/**
 * Get if the object at path is a file or not.
 * Also returns false if it doesn't exist.
 */
bool nvfs_is_file(const char* path);

/**
 * Get if the object at path is a link or not.
 * Also returns false if it doesn't exist.
 */
bool nvfs_is_link(const char* path);

/**
 * fpath must only be a regular file or a link to a regular file.
 * On error, out_size is set to 0, and the error is returned.
 */
nv_error nvfs_file_size(const char* fpath, size_t* out_size);

/**
 * Get the time the entry was last accessed in. On some linux systems, the filesystem can be set to
 * not update the access time every time the entry is accessed. So this need not be the most reliable method.
 * On error, out_size is set to 0, and the error is returned.
 */
nv_error nvfs_get_access_time(const char* fpath, size_t* out_atime);

/**
 * Get the time that the entry was last modified in.
 * On error, out_size is set to 0, and the error is returned.
 */
nv_error nvfs_get_modified_time(const char* fpath, size_t* out_mtime);

/**
 * Read all of the data of a file into a buffer.
 * buffer is allocated through malloc and must be freed by the user.
 * Note that the buffer is NULL terminated and the terminator is not included in buffer_size
 * On error, *buffer is set to NULL and *buffer_size is set to 0.
 */
nv_error nvfs_file_read_all(const char* fpath, char** buffer, size_t* buffer_size);

/**
 * If the user has passed in a directory that does not exist, this function will fail
 * and return NV_ERROR_INVALID_OPERATION. You need to create all parent directories before writing.
 */
nv_error nvfs_file_write_all(const char* fpath, const void* data, size_t data_size);

/**
 * Create an empty file.
 * You will need to create all parent directories before calling this function.
 * The state of the created file on error is not defined.
 */
nv_error nvfs_file_create(const char* fpath);

/**
 * Remove a file or empty directory.
 * If the file does not exist, returns NV_ERROR_NO_EXIST, though technically not an error.
 * It is invalid to attempt to remove non-empty directories through this function.
 */
nv_error nvfs_file_delete(const char* fpath);

/**
 * Must create parent directories before calling this function.
 */
nv_error nvfs_dir_create(const char* dpath, nvfs_permission perms);

/**
 * Recursively create all parent directories of the directory path specified.
 * It is invalid to call this with a 'dpath' that references a file, not a directory as this function
 * will create a directory with the name of the file (and overwrite it).
 */
nv_error nvfs_dir_create_recursive(const char* dpath, nvfs_permission perms);

/**
 * Create the directories for a file, but not the file itself.
 */
nv_error nvfs_dir_create_recursive_for_file(const char* fpath, nvfs_permission perms);

/**
 * WARNING: Removes everything from the directory.
 * Walk through a directory, removing everything in the directory.
 * TODO: Fix this!!! If an error occurs in deleting any entry, we skip over that file and continue deleting.
 * If a file exists in the directory which we do not have permissions to delete, the file will
 * not be deleted and NV_ERROR_INVALID_OPERATION is returned.
 */
nv_error nvfs_dir_delete_recursive(const char* dpath);

/**
 * The list 'entries' and each element's path in the list is allocated through malloc().
 * Directories "." and ".." are excluded in the list.
 * and must be freed by the user.
 * All paths are relative to the directory.
 * No parameter is allowed to be NULL.
 *
 * eg.
 * struct nvfs_entry* entries;
 * size_t nentries;
 * nvfs_dir_list(".", &entries, &nentries);
 * for (size_t i = 0; i < nentries; i++) { nv_free(entries[i].path); }
 * nv_free(entries);
 */
nv_error nvfs_dir_list(const char* dpath, struct nvfs_entry** entries, size_t* nentries);

NOVA_HEADER_END

#endif // NV_STD_FILE_H
