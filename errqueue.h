/*
  MIT License

  Copyright (c) 2025 Tellcode

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

#ifndef __NOVA_ERROR_QUEUE_H__
#define __NOVA_ERROR_QUEUE_H__

#include "stdafx.h"

NOVA_HEADER_START

#ifndef NV_MAX_ERRORS
#  define NV_MAX_ERRORS 16
#endif

#ifndef NV_ERROR_LENGTH
#  define NV_ERROR_LENGTH 256
#endif

typedef char nv_error_t[NV_ERROR_LENGTH];

typedef struct nv_error_queue_t
{
  int front;
  int back;
  /* These must be stored on the stack, what if nv_malloc has an error? */
  nv_error_t errors[NV_MAX_ERRORS];
} nv_error_queue_t;

extern void        nv_error_queue_init(nv_error_queue_t* dst);
extern void        nv_error_queue_destroy(nv_error_queue_t* dst);
extern char*       nv_error_queue_push(nv_error_queue_t* queue);
extern const char* nv_error_queue_pop(nv_error_queue_t* queue);

NOVA_HEADER_END

#endif //__NOVA_ERROR_QUEUE_H__
