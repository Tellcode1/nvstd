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

#ifndef NV_STREAM_H
#define NV_STREAM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "error.h"
#include "stdafx.h"
#include "types.h"

  typedef struct nv_stream nv_stream_t;
  typedef size_t (*nv_stream_read_fn)(nv_stream_t* stm, void* buffer, size_t buffer_size);
  typedef size_t (*nv_stream_write_fn)(nv_stream_t* stm, const void* data, size_t nbyte);
  typedef void (*nv_stream_flush_fn)(nv_stream_t* stm);

  typedef enum nv_seek_pos
  {
    /* start of stream */
    NV_SEEK_START,
    /* current position of cursor */
    NV_SEEK_CURR,
    /* end of stream */
    NV_SEEK_END,
  } nv_seek_pos;

  typedef nv_error (*nv_stream_seek_fn)(ssize_t offset, nv_seek_pos pos, nv_stream_t* stm);

  /**
   * Open a stream to a file.
   * 'mode' and 'path' are passed as is to the C fopen API.
   * Refer to https://man7.org/linux/man-pages/man3/fopen.3.html for your options in mode. Atleast on linux.
   * 'path' must refer to a file within permissions
   * On error: stm will be set to NULL and the function will return an error.
   */
  nv_error nv_open_fstream(const char* path, const char* mode, struct nv_stream** stm);

  /**
   * Open a stream to a memory buffer held by the user.
   * The memory buffer must have a constant size.
   * Reads and writes past the buffers size will error out with EOF.
   * \sa nv_open_bufstream
   */
  nv_error nv_open_memstream(void* buffer, size_t buffer_size, struct nv_stream** stm);

  /**
   * Open a stream to a memory buffer allocated and held by the stream.
   * Internal buffer will be resized to fit writes as needed (but not reads!).
   * WARNING: Make sure init_size is not 0!!!
   */
  nv_error nv_open_bufstream(size_t init_size, struct nv_stream** stm);

  /**
   * Open a pipe stream to a libc FILE.
   * All calls will just be passed over to libc.
   */
  nv_error nv_open_pipestream(FILE* pipe, struct nv_stream** stm);

  /**
   * Open a sink stream.
   * All calls will serve as noops and every getter will return 0 or the invalid / unset value for that.
   */
  nv_error nv_open_sinkstream(struct nv_stream** stm);

  /**
   * Open a custom stream.
   * flush, read and write must not be NULL.
   */
  nv_error nv_open_stream(nv_stream_read_fn read, nv_stream_write_fn write, nv_stream_seek_fn seek, nv_stream_flush_fn flush, void* context, struct nv_stream** stm);

  /**
   * Seek to a position in the stream.
   * origin specifies where to calculate the offset relative to.
   * WARNING: offset is a SIGNED size_t! Negative values are allowed but this is easy to break.
   */
  nv_error nv_stream_seek(ssize_t offset, nv_seek_pos origin, struct nv_stream* stm);

#define nv_sseek nv_stream_seek
#define nv_swrite nv_stream_write
#define nv_sread nv_stream_read
#define nv_sflush nv_stream_flush

  size_t nv_stream_write(const void* data, size_t nbyte, struct nv_stream* stm);

  /**
   * "Put" a character into a stream.
   */
#define nv_fputc nv_stream_putc
  size_t nv_stream_putc(int chr, struct nv_stream* stm);

  /**
   * Read a character from a stream.
   * -1 on error (probably EOF).
   */
#define nv_getc nv_stream_getc
  int nv_stream_getc(struct nv_stream* stm);

  /**
   * "Put" a string into a stream.
   * (libc puts adds a newline too, we don't do that.)
   */
  size_t nv_stream_puts(const char* string, struct nv_stream* stm);

  /**
   * Reads 'byte_size' bytes into the buffer. buffer must not be NULL.
   * On successful read, buffer_size will be returned.
   * If buffer_size is greater than the number of bytes left in the stream,
   * NV_ERROR_EOF will be set and the function will return the number of bytes successfully read.
   */
  size_t nv_stream_read(void* buffer, size_t buffer_size, struct nv_stream* stm);

  /**
   * Flush the stream.
   * All the data will be written to their specified location.
   * Except for memories and buffer streams.
   * Additionally, piped streams will call fflush.
   * Sink streams do nothing.
   */
  void nv_stream_flush(struct nv_stream* stm);

  /**
   * Get the error flag of the stream.
   * Errors in streams are "sticky" a la they aren't flushed unless you explicitly do it.
   * They will stay even if you read them. stream_seterror(stm,NV_SUCCESS) if you explicity handle the error.
   * Generally NV_SUCCESS or NV_ERROR_EOF.
   */
  nv_error nv_stream_error(const struct nv_stream* stm);
  void     nv_stream_seterror(struct nv_stream* stm, nv_error error);

  /**
   * Get the context pointer for the stream.
   * For custom streams, it will be the context pointer passed during creation.
   * For file and piped streams, it will be the FILE pointer
   * For buffer streams, it will be the write pointer (pointer to the next byte to write)
   * On error, (invalid stream?), returns NULL.
   */
  void* nv_stream_get_context(const struct nv_stream* stm);

  /**
   * Close a stream.
   * Stream will be flushed, unless stream is a pipe (to allow for fine grain control).
   */
  void nv_close_stream(struct nv_stream* stm);

#ifdef __cplusplus
}
#endif

#endif // NV_STREAM_H
