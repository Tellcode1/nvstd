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

/* Utilities to classify characters in strings. */
/* An implementation to ctype.h, Notably missing isgraph and ishex(or whatever its called) */

#ifndef __NOVA_CHRCLASS_H__
#define __NOVA_CHRCLASS_H__

#include "stdafx.h"

NOVA_HEADER_START

/**
 * @brief Alphabet?
 */
extern bool nv_chr_isalpha(int chr);

/**
 * @brief Digit of a number?
 */
extern bool nv_chr_isdigit(int chr);

/**
 * @brief Alphabetical/Numerical?
 */
extern bool nv_chr_isalnum(int chr);

/**
 * @brief Space/Tab?
 */
extern bool nv_chr_isblank(int chr);

/**
 * @brief Backslash control sequence?
 */
extern bool nv_chr_iscntrl(int chr);

/**
 * @brief Lowercase alphabet?
 */
extern bool nv_chr_islower(int chr);

/**
 * @brief Uppercase alphabet?
 */
extern bool nv_chr_isupper(int chr);

/**
 * @brief Space, newline or tab?
 */
extern bool nv_chr_isspace(int chr);

/**
 * @brief Punctuation?
 */
extern bool nv_chr_ispunct(int chr);

/**
 * @brief Convert a character to lower. Will return the character if it does not have a lower representation.
 */
extern int nv_chr_tolower(int chr);

/**
 * @brief Convert a character to upper. Will return the character if it does not have a lower representation.
 */
extern int nv_chr_toupper(int chr);

NOVA_HEADER_END

#endif //__NOVA_CHRCLASS_H__
