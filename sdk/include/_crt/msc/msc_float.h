#ifndef _FLOAT_H___
#define _FLOAT_H___

#define FLT_RADIX 2

#define FLT_MANT_DIG 24
#define DBL_MANT_DIG 53
#define LDBL_MANT_DIG DBL_MANT_DIG

#define FLT_DIG 6
#define DBL_DIG 15
#define LDBL_DIG 15

#define FLT_MIN_EXP (-125)
#define DBL_MIN_EXP (-1021)
#define LDBL_MIN_EXP (-1021)

#define FLT_MIN_10_EXP (-37)
#define DBL_MIN_10_EXP (-307)
#define LDBL_MIN_10_EXP (-307)

#define FLT_MAX_EXP 128
#define DBL_MAX_EXP 1024
#define LDBL_MAX_EXP 1024

#define FLT_MAX_10_EXP 38
#define DBL_MAX_10_EXP 308
#define LDBL_MAX_10_EXP 308

#define FLT_MAX 3.402823466e+38F
#define DBL_MAX 1.7976931348623158e+308
#define LDBL_MAX 1.7976931348623158e+308

#define FLT_EPSILON 1.192092896e-07F
#define DBL_EPSILON 2.2204460492503131e-016
#define LDBL_EPSILON 2.2204460492503131e-016

#define FLT_MIN 1.175494351e-38F
#define DBL_MIN 2.2250738585072014e-308
#define LDBL_MIN 2.2250738585072014e-308

#define FLT_ROUNDS 1

#define DBL_DECIMAL_DIG  17                      // # of decimal digits of rounding precision
#define DBL_HAS_SUBNORM  1                       // type does support subnormal numbers
#define _DBL_RADIX       2                       // exponent radix
#define DBL_TRUE_MIN     4.9406564584124654e-324 // min positive value

#define FLT_DECIMAL_DIG  9                       // # of decimal digits of rounding precision
#define FLT_HAS_SUBNORM  1                       // type does support subnormal numbers
#define FLT_GUARD        0
#define FLT_NORMALIZE    0
#define FLT_TRUE_MIN     1.401298464e-45F        // min positive value

#define LDBL_HAS_SUBNORM DBL_HAS_SUBNORM         // type does support subnormal numbers
#define _LDBL_RADIX      _DBL_RADIX              // exponent radix
#define LDBL_TRUE_MIN    DBL_TRUE_MIN            // min positive value

#define DECIMAL_DIG      DBL_DECIMAL_DIG

#endif
