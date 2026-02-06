#include "../include/stream.h"

#include "../include/alloc.h"
#include "../include/stdafx.h"
#include "../include/string.h"
#include "../include/types.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef enum stream_type
{
  STREAM_FILE,
  STREAM_BUFFER,
  STREAM_DYNBUFFER,
  STREAM_PIPE,
  STREAM_CUSTOM,
} stream_type;

struct nv_stream
{
  nv_stream_write_fn write; // MAY NOT BE NULL!
  nv_stream_read_fn  read;  // MAY NOT BE NULL!
  nv_stream_flush_fn flush; // MAY NOT BE NULL!
  nv_stream_seek_fn  seek;  // MAY NOT BE NULL!

  /* Offset for reading */
  size_t read_head;

  nv_error error;

  stream_type type;
  union
  {
    FILE* file;
    struct
    {
      void*  buffer;
      uchar* write;
      size_t buffer_size;
    } buf;
    void* user_ctx;
  } val;
};

size_t
file_write(nv_stream_t* stm, const void* data, size_t nbyte)
{
  FILE*  f                 = stm->val.file;
  size_t read_successfully = fwrite(data, 1, nbyte, f);
  if (read_successfully != nbyte) { nv_stream_seterror(stm, NV_ERROR_EOF); }
  return read_successfully;
}
size_t
file_read(nv_stream_t* stm, void* buffer, size_t buffer_size)
{
  FILE*  f                 = stm->val.file;
  size_t read_successfully = fread(buffer, 1, buffer_size, f);
  if (read_successfully != buffer_size) { nv_stream_seterror(stm, NV_ERROR_EOF); }

  // reduce the head by only the bytes that we read successfully
  stm->read_head -= read_successfully;

  return read_successfully;
}
nv_error
file_seek(ssize_t offset, nv_seek_pos pos, nv_stream_t* stm)
{
  FILE* f = stm->val.file;

  int pos_libc = SEEK_SET;
  switch (pos)
  {
    case NV_SEEK_START: pos_libc = SEEK_SET; break;
    case NV_SEEK_CURR: pos_libc = SEEK_CUR; break;
    case NV_SEEK_END: pos_libc = SEEK_END; break;
  }

  int seek = (fseek(f, (long)offset, pos_libc));

  /* update read head */
  stm->read_head = (size_t)ftell(f);

  if (seek)
  {
    if (feof(f)) { nv_stream_seterror(stm, NV_ERROR_EOF); }
    else
    {
      nv_stream_seterror(stm, NV_ERROR_UNKNOWN);
    }
    return stm->error;
  }

  return NV_SUCCESS;
}
void
file_flush(nv_stream_t* stm)
{
  FILE* f = stm->val.file;
  fflush(f);
}

nv_error
nv_open_fstream(const char* path, const char* mode, struct nv_stream** stm)
{
  if (!path) { return NV_ERROR_NO_EXIST; }
  else if (!mode || !stm) { return NV_ERROR_INVALID_ARG; }

  nv_error e = NV_SUCCESS;

  *stm = nv_stalloc(*stm);
  if (!*stm) return NV_ERROR_MALLOC_FAILED;

  struct nv_stream* stmp = *stm;
  stmp->type             = STREAM_FILE;
  stmp->read             = file_read;
  stmp->write            = file_write;
  stmp->flush            = file_flush;
  stmp->seek             = file_seek;
  stmp->val.file         = fopen(path, mode);

  if (!stmp->val.file)
  {
    if (errno == ENOENT || errno == -ENOENT)
    {
      e = NV_ERROR_NO_EXIST;
      goto err_cleanup;
    }
    else if (errno == EPERM || errno == -EPERM)
    {
      e = NV_ERROR_INSUFFICIENT_PERMISSIONS;
      goto err_cleanup;
    }
    else
    {
      e = NV_ERROR_UNKNOWN;
      goto err_cleanup;
    }
  }

  return NV_SUCCESS;

err_cleanup:
  if ((*stm)->val.file)
  {
    fclose((*stm)->val.file);
    (*stm)->val.file = NULL;
  }
  if (stm && *stm)
  {
    nv_free(*stm);
    *stm = NULL;
  }
  return e;
}

#define buffstream_current_offset(stm) ((stm)->val.buf.write - (uchar*)((stm)->val.buf.buffer))

size_t
membuf_write(nv_stream_t* stm, const void* data, size_t nbyte)
{
  size_t current_offset = buffstream_current_offset(stm);
  size_t left           = (stm->val.buf.buffer_size - current_offset);

  if (nbyte > left || !stm->val.buf.buffer)
  {
    if (stm->type == STREAM_DYNBUFFER)
    {
      void* new_buffer    = nv_realloc(stm->val.buf.buffer, NV_MAX(stm->val.buf.buffer_size * 2, stm->val.buf.buffer_size + nbyte));
      stm->val.buf.buffer = new_buffer;
      if (!new_buffer) return 0;
    }
    else
    {
      nbyte = left;
      nv_stream_seterror(stm, NV_ERROR_EOF);
      return stm->error;
    }
  }

  memcpy(stm->val.buf.write, data, nbyte);

  // Move head forward to however many bytes that we wrote.
  stm->val.buf.write += nbyte;

  return nbyte;
}
size_t
membuf_read(nv_stream_t* stm, void* buffer, size_t buffer_size)
{
  size_t current_offset = stm->read_head;
  size_t left           = (stm->val.buf.buffer_size - current_offset);

  if (buffer_size > left)
  {
    nv_stream_seterror(stm, NV_ERROR_EOF);
    // Read only the bytes left.
    buffer_size = left;
  }

  uchar* head = (uchar*)stm->val.buf.buffer; // buffer ptr
  head += stm->read_head;                    // move buffer ptr to head
  memcpy(buffer, head, buffer_size);

  // move head down
  stm->read_head -= buffer_size;

  return buffer_size;
}
nv_error
membuf_seek(ssize_t offset, nv_seek_pos pos, nv_stream_t* stm)
{
  size_t buf_size = stm->val.buf.buffer_size;

  switch (pos)
  {
    case NV_SEEK_START: break;
    case NV_SEEK_CURR: offset += (ssize_t)stm->read_head; break;
    case NV_SEEK_END: offset += (ssize_t)buf_size; break;
  }

  if (stm->read_head + offset > buf_size)
  {
    nv_stream_seterror(stm, NV_ERROR_EOF);
    return NV_ERROR_EOF;
  }
  stm->read_head = offset;

  return NV_SUCCESS;
}
void
membuf_flush(nv_stream_t* stm)
{
  // NOOP
}

nv_error
nv_open_memstream(void* buffer, size_t buffer_size, struct nv_stream** stm)
{
  if (!buffer || buffer_size == 0 || !stm) { return NV_ERROR_INVALID_ARG; }

  nv_error e = NV_SUCCESS;

  *stm = nv_stalloc(*stm);
  if (!*stm) return NV_ERROR_MALLOC_FAILED;

  struct nv_stream* stmp    = *stm;
  stmp->type                = STREAM_BUFFER;
  stmp->val.buf.buffer      = buffer;
  stmp->val.buf.buffer_size = buffer_size;

  // initialize write head to the buffer.
  stmp->val.buf.write = (uchar*)stmp->val.buf.buffer;

  stmp->read  = membuf_read;
  stmp->write = membuf_write;
  stmp->flush = membuf_flush;
  stmp->seek  = membuf_seek;

  return e;
}

nv_error
nv_open_bufstream(size_t init_size, struct nv_stream** stm)
{
  if (init_size == 0 || !stm) { return NV_ERROR_INVALID_ARG; }

  nv_error e = NV_SUCCESS;

  *stm = nv_stalloc(*stm);
  if (!*stm) return NV_ERROR_MALLOC_FAILED;

  struct nv_stream* stmp    = *stm;
  stmp->type                = STREAM_DYNBUFFER;
  stmp->val.buf.buffer      = nv_zmalloc(init_size);
  stmp->val.buf.buffer_size = init_size;

  if (!stmp->val.buf.buffer)
  {
    nv_free(stmp);
    *stm = NULL;
    return NV_ERROR_MALLOC_FAILED;
  }

  // initialize write head to the buffer.
  stmp->val.buf.write = (uchar*)stmp->val.buf.buffer;

  stmp->read  = membuf_read;
  stmp->write = membuf_write;
  stmp->flush = membuf_flush;
  stmp->seek  = membuf_seek;

  return e;
}

nv_error
nv_open_pipestream(FILE* pipe, struct nv_stream** stm)
{
  if (!pipe || !stm) { return NV_ERROR_INVALID_ARG; }

  nv_error e = NV_SUCCESS;

  *stm = nv_stalloc(*stm);
  if (!*stm) return NV_ERROR_MALLOC_FAILED;

  struct nv_stream* stmp = *stm;
  stmp->type             = STREAM_PIPE;
  stmp->val.file         = pipe;

  if (!stmp->val.buf.buffer)
  {
    nv_free(stmp);
    *stm = NULL;
    return NV_ERROR_MALLOC_FAILED;
  }

  // initialize write head to the buffer.
  stmp->val.buf.write = (uchar*)stmp->val.buf.buffer;

  stmp->read  = file_read;
  stmp->write = file_write;
  stmp->flush = file_flush;
  stmp->seek  = file_seek;

  return e;
}

size_t
sink_write(nv_stream_t* stm, const void* data, size_t nbyte)
{
  (void)nbyte;
  (void)data;
  (void)stm;
  return 0;
}
size_t
sink_read(nv_stream_t* stm, void* buffer, size_t buffer_size)
{
  (void)buffer_size;
  (void)buffer;
  (void)stm;
  return 0;
}
nv_error
sink_seek(ssize_t offset, nv_seek_pos pos, nv_stream_t* stm)
{
  (void)stm;
  (void)pos;
  (void)offset;
  return NV_SUCCESS;
}
void
sink_flush(nv_stream_t* stm)
{
  // NOOP
}

nv_error
nv_open_sinkstream(struct nv_stream** stm)
{
  if (!stm) { return NV_ERROR_INVALID_ARG; }

  nv_error e = NV_SUCCESS;

  *stm = nv_stalloc(*stm);
  if (!*stm) return NV_ERROR_MALLOC_FAILED;

  struct nv_stream* stmp = *stm;
  stmp->type             = STREAM_CUSTOM;
  stmp->read             = sink_read;
  stmp->write            = sink_write;
  stmp->flush            = sink_flush;
  stmp->seek             = sink_seek;
  return e;
}

nv_error
nv_open_stream(nv_stream_read_fn read, nv_stream_write_fn write, nv_stream_seek_fn seek, nv_stream_flush_fn flush, void* context, struct nv_stream** stm)
{
  if (!read || !write || !seek || !flush || !stm) { return NV_ERROR_INVALID_ARG; }

  nv_error e = NV_SUCCESS;

  *stm = nv_stalloc(*stm);
  if (!*stm) return NV_ERROR_MALLOC_FAILED;

  struct nv_stream* stmp = *stm;
  stmp->type             = STREAM_CUSTOM;
  stmp->read             = read;
  stmp->write            = write;
  stmp->flush            = flush;
  stmp->seek             = seek;
  stmp->val.user_ctx     = context;

  return e;
}

void
nv_close_stream(struct nv_stream* stm)
{
  if (!stm) return;

  nv_stream_flush(stm);

  if (stm->type == STREAM_DYNBUFFER && stm->val.buf.buffer != NULL) { nv_free(stm->val.buf.buffer); }

  nv_bzero(stm, sizeof(*stm));
  nv_free(stm);
}

nv_error
nv_stream_seek(ssize_t offset, nv_seek_pos base_offset, struct nv_stream* stm)
{
  return stm->seek(offset, base_offset, stm);
}

size_t
nv_stream_write(const void* data, size_t nbyte, struct nv_stream* stm)
{
  return stm->write(stm, data, nbyte);
}

size_t
nv_stream_read(void* buffer, size_t buffer_size, struct nv_stream* stm)
{
  return stm->read(stm, buffer, buffer_size);
}

void
nv_stream_flush(struct nv_stream* stm)
{
  stm->flush(stm);
}

nv_error
nv_stream_error(const struct nv_stream* stm)
{
  return stm->error;
}

void
nv_stream_seterror(struct nv_stream* stm, nv_error error)
{
  stm->error = error;
}

void*
nv_stream_get_context(const struct nv_stream* stm)
{
  if (!stm) return NULL;
  switch (stm->type)
  {
    case STREAM_PIPE:
    case STREAM_FILE: return (void*)stm->val.file;
    case STREAM_DYNBUFFER:
    case STREAM_BUFFER: return (void*)stm->val.buf.write;
    case STREAM_CUSTOM: return stm->val.user_ctx;
  }
  return NULL;
}

size_t
nv_stream_putc(int chr, struct nv_stream* stm)
{
  uchar tmp = (uchar)chr;
  return stm->write(stm, &tmp, sizeof(uchar));
}

size_t
nv_stream_puts(const char* string, struct nv_stream* stm)
{
  return stm->write(stm, string, nv_strlen(string));
}

int
nv_stream_getc(struct nv_stream* stm)
{
  char byte_read = 0;
  if (stm->read(stm, &byte_read, sizeof(byte_read)) != sizeof(byte_read)) { return -1; }
  return (int)byte_read;
}
