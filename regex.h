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

/**
 * Functions for parsing regex
 */

/**
 * Great sources:
 * https://github.com/codecrafters-io/build-your-own-x?tab=readme-ov-file#build-your-own-regex-engine
 *   -https://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html
 *   -https://swtch.com/~rsc/regexp/regexp1.html
 */

#ifndef __NOVA_REGEX_H__
#define __NOVA_REGEX_H__

/**
 * These explanations are DIRECTLY taken from https://swtch.com/~rsc/regexp/regexp1.html!!!
 * '*' matches a sequence of zero or more strings
 *
 */

#include "stdafx.h"

NOVA_HEADER_START

/**
 * Allow for features given by grep
 */
#ifndef NV_REGEX_ALLOW_EXTENDED_GREP_FEATURES
#  define NV_REGEX_ALLOW_EXTENDED_GREP_FEATURES false
#endif

typedef struct nv_regex_state_t nv_regex_state_t;

/**
 * All the metacharacters used in regex.
 */
#define NV_REGEX_METACHARS "*+?()|"

extern bool nv_regex_parse(nv_regex_state_t* NV_RESTRICT regex, const char* NV_RESTRICT input, size_t len);

extern bool nv_regex_validate(nv_regex_state_t* regex, const char* input, size_t len);

typedef enum nv_regex_exprtype
{
  /**
   * A literal character. Compared directly.
   */
  NV_REGEX_EXPRTYPE_LITERAL = 0,

  /**
   * A '.', stands for any non newline character
   */
  NV_REGEX_EXPRTYPE_MATCH_CHAR = 1,

  /**
   * A '*', stands for a possibly zero number of the character preceding it.
   * For example e* would match eeeeeeeeA up to A, as e* means 'Get me as many e as you have consecutively, then stop'
   */
  NV_REGEX_EXPRTYPE_MATCH_STRING = 2,

  NV_REGEX_EXPRTYPE_
} nv_regex_exprtype;

typedef struct nv_regex_expr_t
{
} nv_regex_expr_t;

typedef struct nv_regex_ast_t
{
} nv_regex_ast_t;

struct nv_regex_state_t
{
  const char* regex_string;

  /**
   * The regex supplies ^
   * The string will be matched from the start of the line
   */
  bool anchored;
};

NOVA_HEADER_END

#endif //__NOVA_REGEX_H__
