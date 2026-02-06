#include "../include/file.h"
#include "../include/alloc.h"
#include "../include/errorcodes.h"
#include "../include/print.h"
#include "../include/stdafx.h"
#include "../include/string.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdbool.h>
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
nvfs_perms_to_win_perms(nvfs_permission perms, bool for_directory)
{
  (void)for_directory;
  switch (perms)
  {
    case NVFS_PERMISSION_EXISTS: return 0;
    case NVFS_PERMISSION_READ_ONLY: return 4;
    case NVFS_PERMISSION_WRITE_ONLY: return 2;
    case NVFS_PERMISSION_READ_WRITE: return 6;
    default: return -1;
  }
}

int
nvfs_perms_to_unix_perms(nvfs_permission perms, bool for_directory)
{
  int mode = -1;

#  ifndef _WIN32
  switch (perms)
  {
    case NVFS_PERMISSION_EXISTS: mode = S_IRUSR | S_IWUSR; break;
    case NVFS_PERMISSION_READ_ONLY: mode = S_IRUSR | S_IRGRP | S_IROTH; break;
    case NVFS_PERMISSION_WRITE_ONLY: mode = S_IWUSR | S_IWGRP | S_IWOTH; break;
    case NVFS_PERMISSION_READ_WRITE: mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; break;
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
nvfs_perms_to_sys_perms(nvfs_permission perms, bool for_directory)
{
#  ifdef _WIN32
  return nvfs_perms_to_win_perms(perms, for_directory);
#  else
  return nvfs_perms_to_unix_perms(perms, for_directory);
#  endif
}

bool
nvfs_entry_exists(const char* fpath)
{
  FILE* file;

  // open the file in read mode ; OS wont create it
  // this also handles permissions and all that stupidity
  file = fopen(fpath, "r");
  if (file)
  {
    fclose(file);
    return true;
  }
  return 0;
}

static inline nv_error
_estat(const char* f, struct STAT* s)
{
  if (STAT(f, s) == 0)
  {
    return NV_SUCCESS;
  }

  /* stat failed. return an error based on the errno. */
  if (errno == ENOENT)
  {
    return NV_ERROR_NO_EXIST;
  }
  else if (errno == EPERM)
  {
    return NV_ERROR_INSUFFICIENT_PERMISSIONS;
  }
  return NV_ERROR_IO_ERROR;
}

nv_error
nvfs_file_size(const char* fpath, size_t* out_size)
{
  *out_size = 0;

  struct STAT st;

  nv_error check = _estat(fpath, &st);
  if (check != NV_SUCCESS)
  {
    nv_raise_and_return(check, "Could not stat file %s", fpath);
  }

  *out_size = (size_t)st.st_size;
  return NV_SUCCESS;
}

nv_error
nvfs_get_access_time(const char* fpath, size_t* out_atime)
{
  *out_atime = 0;

  struct STAT st;

  nv_error check = _estat(fpath, &st);
  if (check != NV_SUCCESS)
  {
    nv_raise_and_return(check, "Could not stat file %s", fpath);
  }

  *out_atime = (size_t)st.st_atime;
  return NV_SUCCESS;
}

nv_error
nvfs_get_modified_time(const char* fpath, size_t* out_mtime)
{
  *out_mtime = 0;

  struct STAT st;

  nv_error check = _estat(fpath, &st);
  if (check != NV_SUCCESS)
  {
    nv_raise_and_return(check, "Could not stat file %s", fpath);
  }

  *out_mtime = (size_t)st.st_mtime;
  return NV_SUCCESS;
}

nv_error
nvfs_file_read_all(const char* fpath, char** buffer, size_t* buffer_size)
{
  if (fpath == NULL)
  {
    nv_raise_and_return(NV_ERROR_INVALID_ARG, "Parameter \"fpath\" is not allowed to be NULL");
  }
  if (buffer == NULL)
  {
    nv_raise_and_return(NV_ERROR_INVALID_ARG, "Parameter \"buffer\" is not allowed to be NULL");
  }

  nv_error code = NV_SUCCESS;
  size_t   size = 0;

  *buffer = NULL;
  if (buffer_size)
  {
    *buffer_size = 0;
  }

  /**
   * r => read
   * b => binary
   */
  FILE* fp = fopen(fpath, "rb");
  if (!fp)
  {
    return NV_ERROR_IO_ERROR;
  }

  if ((code = nvfs_file_size(fpath, &size)) != NV_SUCCESS)
  {
    fclose(fp);
    return code;
  }

  *buffer = nv_zmalloc(size + 1);
  if (!*buffer)
  {
    fclose(fp);
    nv_raise_and_return(NV_ERROR_MALLOC_FAILED, "");
  }

  if (fread(*buffer, 1, size, fp) != size)
  {
    nv_free(*buffer);
    *buffer = NULL;
    fclose(fp);
    /**
     * The size of the file changed between us reading its size and now reading its contents
     */
    nv_raise_error(NV_ERROR_IO_ERROR, "File size changed between queries");
  }

  // NULL term our string
  (*buffer)[size] = '\0';

  if (buffer_size)
  {
    *buffer_size = size;
  }

  fclose(fp);
  return NV_SUCCESS;
}

nv_error
nvfs_file_write_all(const char* fpath, const void* data, size_t data_size)
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
nvfs_file_create(const char* fpath)
{
#  ifndef _WIN32

  int fd = open(fpath, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0755);
  if (fd < 0)
  {
    // return already exist if the file already exists.. duh
    if (errno == EEXIST)
    {
      return NV_ERROR_EXIST;
    }
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
nvfs_file_delete(const char* fpath)
{
  if (remove(fpath) != 0)
  {
    if (errno == ENOENT)
    {
      nv_raise_and_return(NV_ERROR_NO_EXIST, "File to delete does not exist");
    }
    else if (errno == EPERM)
    {
      nv_raise_and_return(NV_ERROR_INSUFFICIENT_PERMISSIONS, "Insufficient permissions to delete file");
    }
    return NV_ERROR_IO_ERROR;
  }
  return NV_SUCCESS;
}

nv_error
nvfs_dir_create(const char* dpath, nvfs_permission perms)
{
#  ifndef _WIN32
  int sysperms = nvfs_perms_to_sys_perms(perms, true);
  if (sysperms == -1)
  {
    nv_raise_and_return(NV_ERROR_INSUFFICIENT_PERMISSIONS, "");
  }

  if (mkdir(dpath, sysperms) != 0)
  {
    if (errno == EEXIST) // directory already exists?
    {
      return NV_ERROR_EXIST;
    }
    return NV_ERROR_IO_ERROR;
  }
#  else
  (void)perms;

  if (!CreateDirectoryA(dpath, NULL))
  {
    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS)
    {
      return NV_ERROR_ALREADY_EXIST;
    }
    return NV_ERROR_IO_ERROR;
  }
#  endif
  return NV_SUCCESS;
}

nv_error
nvfs_dir_create_recursive(const char* dpath, nvfs_permission perms)
{
  nv_error check = NV_SUCCESS;

  const size_t dirlen    = nv_strlen(dpath);
  char*        path_copy = nv_zmalloc(dirlen + 1);
  char*        buffer    = nv_zmalloc(dirlen + 1);
  if (path_copy == NULL || buffer == NULL)
  {
    /**
     * Enough information is provided to the user, we really don't need to add extra.
     */
    nv_raise_and_return(NV_ERROR_MALLOC_FAILED, "");
  }

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

        check = nvfs_dir_create(buffer, perms);
        if (check != NV_SUCCESS)
        {
          nv_raise_error(check, "Error in recursive directory creation");
        }

        *p = '/';
      }
    }

    check = nvfs_dir_create(buffer, perms);
    if (check != NV_SUCCESS)
    {
      nv_raise_error(check, "Error in recursive directory creation");
    }
  }

  check = nvfs_dir_create(buffer, perms);
  if (check != NV_SUCCESS)
  {
    nv_raise_error(check, "Error in recursive directory creation");
  }

  nv_free(path_copy);
  nv_free(buffer);

  return NV_SUCCESS;
}

nv_error
nvfs_dir_create_recursive_for_file(const char* fpath, nvfs_permission perms)
{
  nv_error check = NV_SUCCESS;

  const size_t pathlen   = nv_strlen(fpath);
  char*        path_copy = nv_zmalloc(pathlen + 1);
  char*        buffer    = nv_zmalloc(pathlen + 1);
  if (path_copy == NULL || buffer == NULL)
  {
    /**
     * Enough information is provided to the user, we really don't need to add extra.
     */
    nv_raise_and_return(NV_ERROR_MALLOC_FAILED, "");
  }

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

        if ((check = nvfs_dir_create(buffer, perms)) != NV_SUCCESS)
        {
          nv_free(path_copy);
          nv_free(buffer);
          nv_raise_and_return(check, "Error in recursive directory creation");
        }

        *p = '/';
      }
    }

    if ((check = nvfs_dir_create(buffer, perms)) != NV_SUCCESS)
    {
      nv_free(path_copy);
      nv_free(buffer);
      nv_raise_and_return(check, "Error in recursive directory creation");
    }
  }

  nv_free(path_copy);
  nv_free(buffer);

  return NV_SUCCESS;
}

nv_error
nvfs_dir_delete_recursive(const char* dpath)
{
#  ifndef _WIN32
  DIR* dir = opendir(dpath);
  if (!dir)
  {
    if (errno == ENOENT)
    {
      nv_raise_and_return(NV_ERROR_NO_EXIST, "Directory does not exist");
    }
    else if (errno == EPERM)
    {
      nv_raise_and_return(NV_ERROR_INSUFFICIENT_PERMISSIONS, "Insufficient permissions to delete directory");
    }
    else
    {
      /**
       * This is all we really know without digging into the error codes further.
       */
      nv_raise_and_return(NV_ERROR_IO_ERROR, "");
    }
  }

  char*    fullpath = nv_zmalloc(NV_MAX(PATH_MAX, nv_strlen(dpath)));
  nv_error code     = NV_SUCCESS;

  if (fullpath == NULL)
  {
    nv_raise_and_return(NV_ERROR_MALLOC_FAILED, "");
  }

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
        code = nvfs_dir_delete_recursive(fullpath);
      }
      else
      {
        code = nvfs_file_delete(fullpath);
      }

      if (NV_UNLIKELY(code != NV_SUCCESS))
      {
        closedir(dir);
        nv_free(fullpath);
        nv_raise_and_return(code, "Error in recursive directory deletion");
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
  HANDLE           find_file = FindFirstFileA(pattern, &fd);
  if (find_file == INVALID_HANDLE_VALUE)
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
      code = nvfs_dir_delete_recursive(fullpath);
    }
    else
    {
      DeleteFileA(fullpath);
    }

    if (code != NV_SUCCESS)
    {
      FindClose(find_file);
      return code;
    }
  } while (FindNextFileA(find_file, &fd));
  FindClose(find_file);

  RemoveDirectoryA(dpath);

#  endif

  return NV_SUCCESS;
}

#endif

char*
nvfs_file_extension(const char* path)
{
  if (!path)
  {
    return NULL;
  }

  const char* last_sep = NULL;
  const char* p        = path;

  // find last directory separator
  while (*p)
  {
#if defined(_WIN32)
    if (*p == '/' || *p == '\\')
#else
    if (*p == '/')
#endif
    {
      last_sep = p;
    }
    p++;
  }

  // start scanning after the last separator
  const char* filename  = (last_sep) ? last_sep + 1 : path;
  const char* first_dot = nv_strchr(filename, '.');

  // hidden file (like .gitignore) or no dot at all
  if (!first_dot || first_dot == filename || *(first_dot + 1) == '\0')
  {
    return NULL;
  }

  return nv_strdup(first_dot + 1); // everything after first dot
}

char*
nvfs_file_dir(const char* fpath)
{
  const char* last_slash     = nv_strrchr(fpath, '/');
  const char* last_backslash = nv_strrchr(fpath, '\\');
  const char* separator      = (last_slash > last_backslash) ? last_slash : last_backslash;

  if (separator != NULL)
  {
    size_t dir_len = separator - fpath;
    // everything up to the last seperator
    return nv_substr(fpath, 0, dir_len);
  }

  // no directory separator, must be in the same directory
  return nv_strdup(".");
}

nvfs_type
nvfs_entry_type(const char* path)
{
  struct STAT st;
  if (STAT(path, &st) == 0)
  {
    if (S_ISDIR(st.st_mode))
    {
      return NVFS_DIR;
    }
    else if (S_ISLNK(st.st_mode))
    {
      return NVFS_LINK;
    }
    else if (S_ISREG(st.st_mode))
    {
      return NVFS_FILE;
    }
  }
  return NVFS_UNKNOWN;
}

bool
nvfs_is_directory(const char* path)
{
  return nvfs_entry_type(path) == NVFS_DIR;
}

bool
nvfs_is_file(const char* path)
{
  return nvfs_entry_type(path) == NVFS_FILE;
}

bool
nvfs_is_link(const char* path)
{
  return nvfs_entry_type(path) == NVFS_LINK;
}

nv_error
nvfs_dir_list(const char* dpath, struct nvfs_entry** entries, size_t* nentries)
{
  nv_assert_ptr(dpath);
  nv_assert_ptr(entries);
  nv_assert_ptr(nentries);

  if (nvfs_is_directory(dpath) == false)
  {
    nv_raise_and_return(NV_ERROR_INVALID_ARG, "Argument dpath does not point to a directory");
  }

#ifndef _WIN32
  DIR* dirp = opendir(dpath);
  if (dirp == NULL)
  {
    if (errno == ENOENT)
    {
      nv_raise_and_return(NV_ERROR_NO_EXIST, "Directory does not exist");
    }
    else if (errno == EPERM)
    {
      nv_raise_and_return(NV_ERROR_INSUFFICIENT_PERMISSIONS, "Insufficient permissions to list directory");
    }
    return NV_ERROR_IO_ERROR;
  }

  *nentries = 0;

  const size_t init_list_size = 4;
  size_t       capacity       = init_list_size;

  *entries = nv_zmalloc(sizeof(struct nvfs_entry) * init_list_size);
  if (*entries == NULL)
  {
    nv_raise_and_return(NV_ERROR_MALLOC_FAILED, "malloc for entries of size %zu failed", capacity);
  }

  struct dirent* entry = NULL;
  while ((entry = readdir(dirp)) != NULL)
  {
    if (*nentries >= capacity)
    {
      capacity *= 2;
      *entries = nv_realloc(*entries, sizeof(struct nvfs_entry) * capacity);
      if (*entries == NULL)
      {
        nv_raise_and_return(NV_ERROR_MALLOC_FAILED, "malloc for entries of size %zu failed", capacity);
      }
    }

    struct nvfs_entry* write = &(*entries)[(*nentries)++];

    // duplicate the string and add it to the list.
    write->path = nv_strdup(entry->d_name);

    nvfs_type type = NVFS_UNKNOWN;

    // get the type of the entry.
    switch (entry->d_type)
    {
      case DT_REG: type = NVFS_FILE; break;
      case DT_LNK: type = NVFS_LINK; break;
      case DT_DIR: type = NVFS_DIR; break;
      default: break;
    }
    write->type = type;
  }

  closedir(dirp);
#else
  nv_log_and_abort("TODO: Implement");
#endif

  return NV_SUCCESS;
}

char*
nvfs_win_to_unix_path(const char* path)
{
  char* duped = nv_strdup(path);
  for (char* s = duped; *s; s++)
  {
    if (*s == '\\')
      *s = '/';
  }
  return duped;
}
