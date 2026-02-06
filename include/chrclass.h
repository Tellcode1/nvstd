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
 * @brief Digit of a number?
 */
NOVA_ATTR_CONST static inline bool
nv_isdigit(int chr)
{
  return (chr >= '0' && chr <= '9');
}

/**
 * @brief Alphabet?
 */
NOVA_ATTR_CONST static inline bool
nv_isalpha(int chr)
{
  return (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z');
}

/**
 * @brief Alphabetical/Numerical?
 */
NOVA_ATTR_CONST static inline bool
nv_isalnum(int chr)
{
  return nv_isalpha(chr) || nv_isdigit(chr);
}

/**
 * @brief Space/Tab?
 */
NOVA_ATTR_CONST static inline bool
nv_isblank(int chr)
{
  return (chr == ' ') || (chr == '\t');
}

/**
 * @brief Backslash control sequence?
 */
/* https://en.wikipedia.org/wiki/Control_character */
NOVA_ATTR_CONST static inline bool
nv_iscntrl(int chr)
{
  switch (chr)
  {
    /* \e also exists. non standard. yep. */
    case '\0':
    case '\a':
    case '\b':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r': return true;
    default: return false;
  }
  return false;
}

/**
 * @brief Lowercase alphabet?
 */
NOVA_ATTR_CONST static inline bool
nv_islower(int chr)
{
  return (chr >= 'a' && chr <= 'z');
}

/**
 * @brief Uppercase alphabet?
 */
NOVA_ATTR_CONST static inline bool
nv_isupper(int chr)
{
  return (chr >= 'A' && chr <= 'Z');
}

/**
 * @brief Space, newline or tab?
 */
NOVA_ATTR_CONST static inline bool
nv_isspace(int chr)
{
  return (chr == ' ' || chr == '\n' || chr == '\t');
}

/**
 * @brief Punctuation?
 */
NOVA_ATTR_CONST static inline bool
nv_ispunct(int chr)
{
  /* Generated with
    for (i = 0; i < 256; i++)
    {
      if (ispunct(i))
      {
        printf("%c ", i);
      }
    }
  */
  switch (chr)
  {
    case '!':
    case '\"':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case ';':
    case '?':
    case '@':
    case '[':
    case '\\':
    case ']':
    case '^':
    case '_':
    case '`':
    case '{':
    case '|':
    case '}':
    case '~': return true;
    default: return false;
  }
}

/**
 * @brief Convert a character to lower. Will return the character if it does not have a lower representation.
 */
NOVA_ATTR_CONST static inline int
nv_tolower(int chr)
{
  if (nv_isupper(chr)) { return chr + 32; }
  return chr;
}

/**
 * @brief Convert a character to upper. Will return the character if it does not have a lower representation.
 */
NOVA_ATTR_CONST static inline int
nv_toupper(int chr)
{
  if (nv_islower(chr)) { return chr - 32; /* chr - 32 */ }
  return chr;
}

NOVA_HEADER_END

#endif // NV_STD_CHRCLASS_H
