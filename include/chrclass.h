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

#ifndef NV_STD_CHRCLASS_H
#define NV_STD_CHRCLASS_H

#include "attributes.h"
#include "stdafx.h"

NOVA_HEADER_START

/**
 * @brief Alphabet?
 */
extern bool nv_isalpha(int chr) NOVA_ATTR_CONST;

/**
 * @brief Digit of a number?
 */
extern bool nv_isdigit(int chr) NOVA_ATTR_CONST;

/**
 * @brief Alphabetical/Numerical?
 */
extern bool nv_isalnum(int chr) NOVA_ATTR_CONST;

/**
 * @brief Space/Tab?
 */
extern bool nv_isblank(int chr) NOVA_ATTR_CONST;

/**
 * @brief Backslash control sequence?
 */
extern bool nv_iscntrl(int chr) NOVA_ATTR_CONST;

/**
 * @brief Lowercase alphabet?
 */
extern bool nv_islower(int chr) NOVA_ATTR_CONST;

/**
 * @brief Uppercase alphabet?
 */
extern bool nv_isupper(int chr) NOVA_ATTR_CONST;

/**
 * @brief Space, newline or tab?
 */
extern bool nv_isspace(int chr) NOVA_ATTR_CONST;

/**
 * @brief Punctuation?
 */
extern bool nv_ispunct(int chr) NOVA_ATTR_CONST;

/**
 * @brief Convert a character to lower. Will return the character if it does not have a lower representation.
 */
extern int nv_tolower(int chr) NOVA_ATTR_CONST;

/**
 * @brief Convert a character to upper. Will return the character if it does not have a lower representation.
 */
extern int nv_toupper(int chr) NOVA_ATTR_CONST;

NOVA_HEADER_END

#endif // NV_STD_CHRCLASS_H
