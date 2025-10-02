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

#ifndef STD_STACKALLOC_H
#define STD_STACKALLOC_H

#include "stdafx.h"
#include "types.h"
#include <stddef.h>

NOVA_HEADER_START

#define NOVA_STACK_SIZE 4096 // bytes

#define NOVA_SETUP_STACK                                                                                                                                                      \
  uchar  _nv_stack[NOVA_STACK_SIZE] = {};                                                                                                                                     \
  size_t _nv_stack_bumper           = 0
#define nv_stackalloc(size_bytes) (_nv_stackalloc_actual(_nv_stack, &_nv_stack_bumper, size_bytes)) // uhh ugly but single liner :3

void*
nv_stackalloc_actual(uchar* stack, size_t* stack_bumper, size_t size_bytes)
{
  void* ptr = stack + *stack_bumper;
  *stack_bumper += size_bytes;
  return ptr;
}

NOVA_HEADER_END

#endif // STD_STACKALLOC_H
