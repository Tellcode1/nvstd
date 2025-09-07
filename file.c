#include "file.h"
#include "errorcodes.h"
#include "alloc.h"
#include "print.h"
#include "stdafx.h"
#include "string.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>

#if (NOVA_FILE_API_AVAILABLE)

#  ifdef WIN32
#    include <io.h>
#    include <sys/stat.h>
#    include <windows.h>
#    define access _access
#    define FSTAT _fstat
#    define STAT stat
#    define FILENO _filen
#  else
#    include <dirent.h>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <unistd.h>
#    define FSTAT fstat
#    define STAT stat
#    define FILENO fileno
#  endif

int
nv_fs_perms_to_win_perms(nv_fs_permission perms, bool for_directory)
{
  (void)for_directory;
  switch (perms)
  {
    case NV_FS_PERMISSION_EXISTS: return 0;
    case NV_FS_PERMISSION_READ_ONLY: return 4;
    case NV_FS_PERMISSION_WRITE_ONLY: return 2;
    case NV_FS_PERMISSION_READ_WRITE: return 6;
    default: return -1;
  }
}

int
nv_fs_perms_to_unix_perms(nv_fs_permission perms, bool for_directory)
{
  int mode = -1;

#  ifndef _WIN32
  switch (perms)
  {
    case NV_FS_PERMISSION_EXISTS: mode = S_IRUSR | S_IWUSR; break;
    case NV_FS_PERMISSION_READ_ONLY: mode = S_IRUSR | S_IRGRP | S_IROTH; break;
    case NV_FS_PERMISSION_WRITE_ONLY: mode = S_IWUSR | S_IWGRP | S_IWOTH; break;
    case NV_FS_PERMISSION_READ_WRITE: mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; break;
    default: mode = S_IRUSR | S_IWUSR; break;
  }

  if (for_directory)
  {
    if (mode & S_IRUSR)
    {
      mode |= S_IXUSR;
    }
    if (mode & S_IRGRP)
    {
      mode |= S_IXGRP;
    }
    if (mode & S_IROTH)
    {
      mode |= S_IXOTH;
    }
  }
#  endif

  return mode;
}

static inline int
nv_fs_perms_to_sys_perms(nv_fs_permission perms, bool for_directory)
{
#  ifdef _WIN32
  return nv_fs_perms_to_win_perms(perms, for_directory);
#  else
  return nv_fs_perms_to_unix_perms(perms, for_directory);
#  endif
}

bool
nv_fs_file_exists(const char* fpath)
{
  return (access(fpath, nv_fs_perms_to_sys_perms(NV_FS_PERMISSION_EXISTS, false)) == 0);
}

nv_error
nv_fs_file_size(const char* fpath, size_t* out_size)
{
  struct STAT st;
  if (STAT(fpath, &st) == 0)
  {
    *out_size = (size_t)st.st_size;
    return NV_SUCCESS;
  }
  *out_size = 0;
  return NV_ERROR_IO_ERROR;
}

nv_error
nv_fs_file_get_access_time(const char* fpath, size_t* out_atime)
{
  struct STAT st;
  if (STAT(fpath, &st) == 0)
  {
    *out_atime = (size_t)st.st_atime;
    return NV_SUCCESS;
  }
  *out_atime = 0;
  return NV_ERROR_IO_ERROR;
}

nv_error
nv_fs_file_get_modified_time(const char* fpath, size_t* out_atime)
{
  struct STAT st;
  if (STAT(fpath, &st) == 0)
  {
    *out_atime = (size_t)st.st_mtime;
    return NV_SUCCESS;
  }
  *out_atime = 0;
  return NV_ERROR_IO_ERROR;
}

nv_error
nv_fs_file_read_all(const char* fpath, nv_allocator_fn alloc, void* alloc_arg, char** buffer, size_t* buffer_size)
{
  nv_error code = NV_SUCCESS;
  size_t   size = 0;

  *buffer = NULL;
  if (buffer_size)
  {
    *buffer_size = 0;
  }

  FILE* fp = fopen(fpath, "rbe");
  if (!fp)
  {
    return NV_ERROR_IO_ERROR;
  }

  if ((code = nv_fs_file_size(fpath, &size)) != NV_SUCCESS)
  {
    fclose(fp);
    return code;
  }

  *buffer = alloc(alloc_arg, NULL, NV_ALLOC_NEW_BLOCK, size + 1);
  if (!*buffer)
  {
    fclose(fp);
    return NV_ERROR_MALLOC_FAILED;
  }

  if (fread(*buffer, 1, size, fp) != size)
  {
    alloc(alloc_arg, *buffer, size + 1, NV_ALLOC_FREE);
    *buffer = NULL;
    fclose(fp);
    return NV_ERROR_IO_ERROR;
  }

  (*buffer)[size] = '\0';

  if (buffer_size)
  {
    *buffer_size = size;
  }

  fclose(fp);
  return NV_SUCCESS;
}

nv_error
nv_fs_file_write_all(const char* fpath, const void* data, size_t data_size)
{
  FILE* fp = fopen(fpath, "wbe");
  if (!fp)
  {
    return NV_ERROR_IO_ERROR;
  }

  if (fwrite(data, 1, data_size, fp) != data_size)
  {
    fclose(fp);
    return NV_ERROR_IO_ERROR;
  }

  fclose(fp);
  return NV_SUCCESS;
}

nv_error
nv_fs_file_create(const char* fpath)
{
#  ifndef _WIN32

  int fd = open(fpath, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0755);
  if (fd < 0 && errno != EEXIST)
  {
    return NV_ERROR_IO_ERROR;
  }
  close(fd);

#  else

  HANDLE bimbows_file_handle = CreateFileA(fpath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

  if (bimbows_file_handle == INVALID_HANDLE_VALUE)
  {
    DWORD error = GetLastError();
    if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
    {
      return NV_ERROR_IO_ERROR;
    }
  }
  else
  {
    CloseHandle(bimbows_file_handle);
  }

#  endif

  return NV_SUCCESS;
}

nv_error
nv_fs_file_delete(const char* fpath)
{
  if (remove(fpath) != 0)
  {
    return NV_ERROR_IO_ERROR;
  }
  return NV_SUCCESS;
}

nv_error
nv_fs_dir_create(const char* dpath, nv_fs_permission perms)
{
#  ifndef _WIN32
  if (mkdir(dpath, nv_fs_perms_to_sys_perms(perms, true)) != 0 && errno != EEXIST)
  {
    return NV_ERROR_IO_ERROR;
  }
#  else
  (void)perms;

  if (!CreateDirectoryA(dpath, NULL))
  {
    DWORD err = GetLastError();
    if (err != ERROR_ALREADY_EXISTS)
    {
      return NV_ERROR_IO_ERROR;
    }
  }
#  endif
  return NV_SUCCESS;
}

nv_error
nv_fs_dir_create_recursive(const char* dpath, nv_fs_permission perms)
{
  const size_t dirlen    = nv_strlen(dpath);
  char*        path_copy = nv_calloc(dirlen + 1);
  char*        buffer    = nv_calloc(dirlen + 1);

  nv_strlcpy(path_copy, dpath, dirlen + 1);

  char* last_separator = nv_strrchr(path_copy, '/');
  if (last_separator != NULL)
  {
    *last_separator = '\0';

    nv_strlcpy(buffer, path_copy, dirlen + 1);
    size_t len = nv_strlen(buffer);

    if (buffer[len - 1] == '/')
    {
      buffer[len - 1] = 0;
    }

    for (char* p = buffer + 1; *p; p++)
    {
      if (*p == '/')
      {
        *p = 0;
        nv_fs_dir_create(buffer, perms);
        *p = '/';
      }
    }

    nv_fs_dir_create(buffer, perms);
  }

  nv_fs_dir_create(buffer, perms);
  nv_free(path_copy);
  nv_free(buffer);

  return NV_SUCCESS;
}

nv_error
nv_fs_dir_create_recursive_for_file(const char* fpath, nv_fs_permission perms)
{
  const size_t pathlen   = nv_strlen(fpath);
  char*        path_copy = nv_calloc(pathlen + 1);
  char*        buffer    = nv_calloc(pathlen + 1);

  nv_strlcpy(path_copy, fpath, pathlen + 1);

  char* last_separator = nv_strrchr(path_copy, '/');
  if (last_separator != NULL)
  {
    *last_separator = '\0';

    nv_strlcpy(buffer, path_copy, pathlen + 1);
    size_t len = nv_strlen(buffer);

    if (buffer[len - 1] == '/')
    {
      buffer[len - 1] = 0;
    }

    char* start = buffer;
    if (buffer[0] == '/')
    {
      start = buffer + 1;
    }

    for (char* p = start; *p; p++)
    {
      if (*p == '/')
      {
        *p = 0;

        if (nv_fs_dir_create(buffer, perms) != NV_SUCCESS)
        {
          nv_free(path_copy);
          nv_free(buffer);
          return NV_ERROR_IO_ERROR;
        }

        *p = '/';
      }
    }

    if (nv_fs_dir_create(buffer, perms) != NV_SUCCESS)
    {
      nv_free(path_copy);
      nv_free(buffer);
      return NV_ERROR_IO_ERROR;
    }
  }

  nv_free(path_copy);
  nv_free(buffer);

  return NV_SUCCESS;
}

nv_error
nv_fs_dir_delete_recursive(const char* dpath)
{
#  ifndef _WIN32
  DIR* dir = opendir(dpath);
  if (!dir)
  {
    return NV_ERROR_IO_ERROR;
  }

  char*    fullpath = nv_calloc(NV_MAX(PATH_MAX, nv_strlen(dpath)));
  nv_error code     = NV_SUCCESS;

  struct dirent* entry = NULL;
  while ((entry = readdir(dir)) != NULL)
  {
    if (nv_strcmp(entry->d_name, ".") == 0 || nv_strcmp(entry->d_name, "..") == 0)
    {
      continue;
    }

    nv_snprintf(fullpath, sizeof(fullpath), "%s/%s", dpath, entry->d_name);

    struct stat st;
    if (stat(fullpath, &st) == 0)
    {
      if (S_ISDIR(st.st_mode))
      {
        code = nv_fs_dir_delete_recursive(fullpath);
      }
      else
      {
        code = nv_fs_file_delete(fullpath);
      }

      if (NV_UNLIKELY(code != NV_SUCCESS))
      {
        closedir(dir);
        nv_free(fullpath);
        return code;
      }
    }
  }
  closedir(dir);
  remove(dpath);

  nv_free(fullpath);

#  else

  char pattern[MAX_PATH];
  nv_snprintf(pattern, MAX_PATH, "%s\\*", dpath);

  WIN32_FIND_DATAA fd;
  HANDLE           bimbows_find_file = FindFirstFileA(pattern, &fd);
  if (bimbows_find_file == INVALID_HANDLE_VALUE)
  {
    return NV_ERROR_IO_ERROR;
  }

  nv_error code = NV_SUCCESS;

  do
  {
    if (nv_strcmp(fd.cFileName, ".") == 0 || nv_strcmp(fd.cFileName, "..") == 0)
    {
      continue;
    }

    char fullpath[MAX_PATH];
    nv_snprintf(fullpath, MAX_PATH, "%s\\%s", dpath, fd.cFileName);

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      code = nv_fs_dir_delete_recursive(fullpath);
    }
    else
    {
      DeleteFileA(fullpath);
    }

    if (code != NV_SUCCESS)
    {
      FindClose(bimbows_find_file);
      return code;
    }
  } while (FindNextFileA(bimbows_find_file, &fd));
  FindClose(bimbows_find_file);

  RemoveDirectoryA(dpath);

#  endif

  return NV_SUCCESS;
}

#endif
