/**
 * Copyright © 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _FLOAT_H
#define _FLOAT_H

#ifdef __cplusplus
extern "C" {
#endif

int __flt_rounds(void);
#define FLT_ROUNDS (__flt_rounds())

#define FLT_RADIX 2

#define FLT_TRUE_MIN 1.40129846432481707092e-45F
#define FLT_MIN 1.17549435082228750797e-38F
#define FLT_MAX 3.40282346638528859812e+38F
#define FLT_EPSILON 1.1920928955078125e-07F

#define FLT_MANT_DIG 24
#define FLT_MIN_EXP (-125)
#define FLT_MAX_EXP 128
#define FLT_HAS_SUBNORM 1

#define FLT_DIG 6
#define FLT_DECIMAL_DIG 9
#define FLT_MIN_10_EXP (-37)
#define FLT_MAX_10_EXP 38

#define DBL_TRUE_MIN 4.94065645841246544177e-324
#define DBL_MIN 2.22507385850720138309e-308
#define DBL_MAX 1.79769313486231570815e+308
#define DBL_EPSILON 2.22044604925031308085e-16

#define DBL_MANT_DIG 53
#define DBL_MIN_EXP (-1021)
#define DBL_MAX_EXP 1024
#define DBL_HAS_SUBNORM 1

#define DBL_DIG 15
#define DBL_DECIMAL_DIG 17
#define DBL_MIN_10_EXP (-307)
#define DBL_MAX_10_EXP 308

#define LDBL_HAS_SUBNORM 1
#define LDBL_DECIMAL_DIG DECIMAL_DIG

#define FLT_EVAL_METHOD 0

#define LDBL_TRUE_MIN 6.47517511943802511092443895822764655e-4966L
#define LDBL_MIN 3.36210314311209350626267781732175260e-4932L
#define LDBL_MAX 1.18973149535723176508575932662800702e+4932L
#define LDBL_EPSILON 1.92592994438723585305597794258492732e-34L

#define LDBL_MANT_DIG 113
#define LDBL_MIN_EXP (-16381)
#define LDBL_MAX_EXP 16384

#define LDBL_DIG 33
#define LDBL_MIN_10_EXP (-4931)
#define LDBL_MAX_10_EXP 4932

#define DECIMAL_DIG 36

#ifdef __cplusplus
}
#endif

#endif
