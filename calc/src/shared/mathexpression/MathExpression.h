/*
 * MathExpression.h
 *
 *  Created on : May 1, 2009
 *
 *  Authors    : Andrey Myznikov <andrey.myznikov@gmail.com>
 *               Elizaveta Evstifeeva <e.evstifeeva@gmail.com>
 *               Sergey Prokopenko <serg.v.prokopenko@gmail.com>
 *
 *  Last Update: Jun 23 2009
 *
 * Here are 2 examples of customized MathExpression class parametrized by float and double.
 * Note that at different platforms some math functions could be not defined.
 * If this is your case, then just comment out corresponding lines, or provide appropriate 
 * equivalents from your platform math library.
 *
 * NOTE:
 * The use of ugly macro ADD_FUNCTION* and ADD_OPERATOR is a way to workaround gedanken MSVC compiler.
 *
 */

#ifndef MATHEXPRESSION_H_
#define MATHEXPRESSION_H_

#ifdef _MSC_VER
  #define _USE_MATH_DEFINES
#endif

#include "MathExpressionBase.h"
#include <math.h>
#include <float.h>


#ifdef _MSC_VER
  #define ADD_FUNCTION1(f,name,description)       add_function(name,f,description)
  #define ADD_FUNCTION2(type,f,name,description)  add_function(name,f,description)
  #define ADD_OPERATOR(f,name)                    add(name,f)
#else
  // prefer to use the template overloading due to performance issues
  #define ADD_FUNCTION1(f,name,description)       add_function<f>(name,description)
  #define ADD_FUNCTION2(type,f,name,description)  add_function<type,f>(name,description)
  #define ADD_OPERATOR(f,name)                    add<f>(name)
#endif


#ifdef _MSC_VER
  #define hypot _hypot
  #define j0    _j0
  #define j1    _j1
  #define y0    _y0
  #define y1    _y1
  #define jn    _j0
  #define yn    _yn
#endif


template<class T>
class MathExpression;



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MathExpression<double>

template<>
class MathExpression<double>
  : public MathExpressionBase<double>
  {
public:
  typedef double value_type;

  MathExpression()
    {
    BinaryOpTable.set_num_levels(7);
    BinaryOpTable[0]->ADD_OPERATOR(operator_logical_or,"||");
    BinaryOpTable[1]->ADD_OPERATOR(operator_logical_and,"&&");
    BinaryOpTable[2]->ADD_OPERATOR(operator_eq,"==");
    BinaryOpTable[2]->ADD_OPERATOR(operator_not_eq,"!=");
    BinaryOpTable[3]->ADD_OPERATOR(operator_lt,"<" );
    BinaryOpTable[3]->ADD_OPERATOR(operator_le,"<=");
    BinaryOpTable[3]->ADD_OPERATOR(operator_gt,">" );
    BinaryOpTable[3]->ADD_OPERATOR(operator_ge,">=");
    BinaryOpTable[4]->ADD_OPERATOR(operator_plus,"+" );
    BinaryOpTable[4]->ADD_OPERATOR(operator_minus,"-" );
    BinaryOpTable[5]->ADD_OPERATOR(operator_multiply,"*" );
    BinaryOpTable[5]->ADD_OPERATOR(operator_divide,"/");
    BinaryOpTable[6]->ADD_OPERATOR(pow,"**");

    UnaryOpTable.ADD_OPERATOR(operator_unary_minus,"-");
    UnaryOpTable.ADD_OPERATOR(operator_unary_plus,"+");
    UnaryOpTable.ADD_OPERATOR(operator_logical_not,"!");

    add_named_constant("PI",M_PI);
    add_named_constant("pi",M_PI);
    add_named_constant("E",M_E);
    add_named_constant("e",M_E);
    add_named_constant("eps",DBL_EPSILON);

    ADD_FUNCTION1(sinc        ,"sinc"   ,"sinc(x)/x");
    ADD_FUNCTION1(gexp        ,"gexp"   ,"exp(-x^2/2)");
    ADD_FUNCTION1(rand        ,"rand"   ,"rand() - uniform random number in the range [0..1]");

    ADD_FUNCTION1(fabs        ,"abs"    ,"abs(x) Absolute value of X. ");
    ADD_FUNCTION1(acos        ,"acos"   ,"acos(x) Arc cosine of X.  ");
    ADD_FUNCTION1(asin        ,"asin"   ,"asin(x) Arc sine of X.");
    ADD_FUNCTION1(atan        ,"atan"   ,"atan(x) Arc tangent of X.");
    ADD_FUNCTION1(atan2       ,"atan2"  ,"atan2(x,y) Arc tangent of Y/X.");
    ADD_FUNCTION1(cos         ,"cos"    ,"cos(x) Cosine of X.");
    ADD_FUNCTION1(sin         ,"sin"    ,"sin(x) Sine of X.");
    ADD_FUNCTION1(tan         ,"tan"    ,"tan(x) Tangent of X.");
    ADD_FUNCTION1(cosh        ,"cosh"   ,"cosh(x) Hyperbolic cosine of X.");
    ADD_FUNCTION1(sinh        ,"sinh"   ,"sinh(x) Hyperbolic sine of X.");
    ADD_FUNCTION1(tanh        ,"tanh"   ,"tanh(x) Hyperbolic tangent of X.");
    ADD_FUNCTION1(exp         ,"exp"    ,"exp(x) Exponential function of X.");
    ADD_FUNCTION1(log         ,"ln"     ,"ln(x) Natural logarithm of X.");
    ADD_FUNCTION1(log         ,"log"    ,"log(x) Natural logarithm of X.");
    ADD_FUNCTION1(log10       ,"lg"     ,"lg(x) Base-ten logarithm of X.");
    ADD_FUNCTION1(log10       ,"log10"  ,"log10(x) Base-ten logarithm of X.");
    ADD_FUNCTION1(pow         ,"pow"    ,"pow(x,y) Return X to the Y power.");
    ADD_FUNCTION1(sqrt        ,"sqrt"   ,"sqrt(x) The square root of X.  ");
    ADD_FUNCTION1(hypot       ,"hypot"  ,"hypot(x,y) Return `sqrt(X*X + Y*Y)'.");
    ADD_FUNCTION1(ceil        ,"ceil"   ,"ceil(x) Smallest integral value not less than X.");
    ADD_FUNCTION1(floor       ,"floor"  ,"floor(x) Largest integer not greater than X.");
    ADD_FUNCTION1(fmod        ,"fmod"   ,"fmod(x,y) Floating-point modulo remainder of X/Y.");
    ADD_FUNCTION1(j0          ,"j0"     ,"The j0(x) bessel function");
    ADD_FUNCTION1(j1          ,"j1"     ,"The j1(x) bessel function");
    ADD_FUNCTION1(y0          ,"y0"     ,"The y0(x) bessel function");
    ADD_FUNCTION1(y1          ,"y1"     ,"The y1(x) bessel function");

    ADD_FUNCTION2(double(*)(int,double),jn,"jn","jn(n,x) bessel function");
    ADD_FUNCTION2(double(*)(int,double),yn,"yn","yn(n,x) bessel function");

    add_function("sum",     sum,      "sum(arg1,...) the sum of arguments");
    add_function("product", product,  "product(arg1,...) the product of arguments");
    add_function("min",     min,      "min(arg1,...) the min of arguments");
    add_function("max",     max,      "max(arg1,...) the max of arguments");

#ifndef _MSC_VER
    ADD_FUNCTION1(acosh       ,"acosh"  ,"acosh(x) Hyperbolic arc cosine of X.");
    ADD_FUNCTION1(asinh       ,"asinh"  ,"asinh(x) Hyperbolic arc sine of X.");
    ADD_FUNCTION1(atanh       ,"atanh"  ,"atanh(x) Hyperbolic arc tangent of X.");
    ADD_FUNCTION1(expm1       ,"expm1"  ,"expm1(x) Return exp(X) - 1.");
    ADD_FUNCTION1(cbrt        ,"cbrt"   ,"cbrt(x) The cube root of X.");
    ADD_FUNCTION1(drem        ,"drem"   ,"drem(x,y) Return the remainder of X/Y.");
    ADD_FUNCTION1(erf         ,"erf"    ,"erf(x) Error function");
    ADD_FUNCTION1(erfc        ,"erfc"   ,"The erfc function");
    ADD_FUNCTION1(lgamma      ,"gamma"  ,"The gamma(x) function");
    ADD_FUNCTION1(tgamma      ,"tgamma" ,"The true gamma function tgamma(x)");
    ADD_FUNCTION1(exp2        ,"exp2"   ,"exp2(x) base-2 exponential of X.");
    ADD_FUNCTION1(log2        ,"log2"   ,"log2(x) base-2 logarithm of X.");
    ADD_FUNCTION1(round       ,"round"  ,"round(x) Round X to nearest integral value, rounding halfway cases away from zero.");
    ADD_FUNCTION1(trunc       ,"trunc"  ,"trunc(x) Round X to the integral value in floating-point format nearest but not larger in magnitude.");
    ADD_FUNCTION1(fdim        ,"fdim"   ,"fdim(x,y) Return positive difference between X and Y.");
    ADD_FUNCTION1(fmax        ,"fmax"   ,"fmax(x,y) Return maximum numeric value from X and Y");
    ADD_FUNCTION1(fmin        ,"fmin"   ,"fmin(x,y) Return maximum numeric value from X and Y");
    ADD_FUNCTION1(fma         ,"fma"    ,"fma(x,y,z) Multiply-add function computed as a ternary operation.");
    ADD_FUNCTION1(scalb       ,"scalb"  ,"scalb(x,n) Return X times (2 to the Nth power).");
    ADD_FUNCTION1(log1p       ,"log1p"  ,"log1p(x) Return log(1 + X).");
    ADD_FUNCTION1(logb        ,"logb"   ,"The logb(x) base 2 signed integral exponent of X.");
    ADD_FUNCTION1(significand ,"significand","significand(x) Return the fractional part of X after dividing out `ilogb (X)'.");
    ADD_FUNCTION1(copysign    ,"copysign","copysign(x,y) Return X with its signed changed to Y's.");
    ADD_FUNCTION1(rint        ,"rint"   ,"rint(x) Return the integer nearest X in the direction of the prevailing rounding mode.");
    ADD_FUNCTION1(nextafter   ,"nextafter","nextafter(x,y) Return X + epsilon if X < Y, X - epsilon if X > Y.");
    ADD_FUNCTION1(remainder   ,"remainder","remainder(x,y) Return the remainder of integer division X / Y with infinite precision.");
    ADD_FUNCTION1(nearbyint   ,"nearbyint","nearbyint(x) Round X to integral value in floating-point format using current rounding direction");
    ADD_FUNCTION2(int(*)(double),isinf,"isinf","isinf(x) Return 0 if VALUE is finite or NaN, +1 if it is +Infinity, -1 if it is -Infinity. ");
    ADD_FUNCTION2(int(*)(double),finite,"finite","finite(x) Return nonzero if VALUE is finite and not NaN.");
    ADD_FUNCTION2(int(*)(double),isnan,"isnan","isnan(x) Return nonzero if VALUE is not a number.");
    ADD_FUNCTION2(double(*)(double,int),scalbn,"scalbn","The scalbn(x, int n ) Return X times (2 to the Nth power).");
    ADD_FUNCTION2(double(*)(double,long),scalbln,"scalbln","The scalbln(x, long n) Return X times (2 to the Nth power).");
#endif

    }

protected:
  bool parse_number( double * value, const char * curpos, char ** endptr )
    { * value = strtod(curpos,endptr);
      return *endptr>curpos;
    }
  static double sinc(double x)
    { return x?sin(x)/x:0;
    }
  static double gexp(double x)
    { return exp(-x*x/2);
    }
  static double rand()
    { return (double)::rand()/RAND_MAX;
    }
  };


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MathExpression<float>

template<>
class MathExpression<float>
  : public MathExpressionBase<float>
  {
public:

  typedef float value_type;

  MathExpression()
    {
    BinaryOpTable.set_num_levels(7);
    BinaryOpTable[0]->ADD_OPERATOR(operator_logical_or,"||");
    BinaryOpTable[1]->ADD_OPERATOR(operator_logical_and,"&&");
    BinaryOpTable[2]->ADD_OPERATOR(operator_eq,"==");
    BinaryOpTable[2]->ADD_OPERATOR(operator_not_eq,"!=");
    BinaryOpTable[3]->ADD_OPERATOR(operator_lt,"<" );
    BinaryOpTable[3]->ADD_OPERATOR(operator_le,"<=");
    BinaryOpTable[3]->ADD_OPERATOR(operator_gt,">" );
    BinaryOpTable[3]->ADD_OPERATOR(operator_ge,">=");
    BinaryOpTable[4]->ADD_OPERATOR(operator_plus,"+" );
    BinaryOpTable[4]->ADD_OPERATOR(operator_minus,"-" );
    BinaryOpTable[5]->ADD_OPERATOR(operator_multiply,"*" );
    BinaryOpTable[5]->ADD_OPERATOR(operator_divide,"/");
    BinaryOpTable[6]->ADD_OPERATOR(powf,"**");

    UnaryOpTable.ADD_OPERATOR(operator_unary_minus,"-");
    UnaryOpTable.ADD_OPERATOR(operator_unary_plus,"+");
    UnaryOpTable.ADD_OPERATOR(operator_logical_not,"!");

    add_named_constant("PI",M_PI);
    add_named_constant("pi",M_PI);
    add_named_constant("E",M_E);
    add_named_constant("e",M_E);
    add_named_constant("eps",FLT_EPSILON);

    ADD_FUNCTION1(sinc        ,"sinc"   ,"sinc(x)/x");
    ADD_FUNCTION1(gexp        ,"gexp"   ,"exp(-x^2/2)");
    ADD_FUNCTION1(rand        ,"rand"   ,"rand() - uniform random number in the range [0..1]");

    ADD_FUNCTION1(fabsf       ,"abs"    ,"abs(x) Absolute value of X. ");
    ADD_FUNCTION1(acosf       ,"acos"   ,"acos(x) Arc cosine of X.  ");
    ADD_FUNCTION1(asinf       ,"asin"   ,"asin(x) Arc sine of X.");
    ADD_FUNCTION1(atanf       ,"atan"   ,"atan(x) Arc tangent of X.");
    ADD_FUNCTION1(atan2f      ,"atan2"  ,"atan2(x,y) Arc tangent of Y/X.");
    ADD_FUNCTION1(cosf        ,"cos"    ,"cos(x) Cosine of X.");
    ADD_FUNCTION1(sinf        ,"sin"    ,"sin(x) Sine of X.");
    ADD_FUNCTION1(tanf        ,"tan"    ,"tan(x) Tangent of X.");
    ADD_FUNCTION1(coshf       ,"cosh"   ,"cosh(x) Hyperbolic cosine of X.");
    ADD_FUNCTION1(sinhf       ,"sinh"   ,"sinh(x) Hyperbolic sine of X.");
    ADD_FUNCTION1(tanhf       ,"tanh"   ,"tanh(x) Hyperbolic tangent of X.");
    ADD_FUNCTION1(expf        ,"exp"    ,"exp(x) Exponential function of X.");
    ADD_FUNCTION1(logf        ,"ln"     ,"ln(x) Natural logarithm of X.");
    ADD_FUNCTION1(logf        ,"log"    ,"log(x) Natural logarithm of X.");
    ADD_FUNCTION1(log10f      ,"lg"     ,"lg(x) Base-ten logarithm of X.");
    ADD_FUNCTION1(log10f      ,"log10"  ,"log10(x) Base-ten logarithm of X.");
    ADD_FUNCTION1(powf        ,"pow"    ,"pow(x,y) Return X to the Y power.");
    ADD_FUNCTION1(sqrtf       ,"sqrt"   ,"sqrt(x) The square root of X.  ");
    ADD_FUNCTION1(hypotf      ,"hypot"  ,"hypot(x,y) Return `sqrt(X*X + Y*Y)'.");
    ADD_FUNCTION1(ceilf       ,"ceil"   ,"ceil(x) Smallest integral value not less than X.");
    ADD_FUNCTION1(floorf      ,"floor"  ,"floor(x) Largest integer not greater than X.");
    ADD_FUNCTION1(fmodf       ,"fmod"   ,"fmod(x,y) Floating-point modulo remainder of X/Y.");
    ADD_FUNCTION1(j0f         ,"j0"     ,"The j0(x) bessel function");
    ADD_FUNCTION1(j1f         ,"j1"     ,"The j1(x) bessel function");
    ADD_FUNCTION1(y0f         ,"y0"     ,"The y0(x) bessel function");
    ADD_FUNCTION1(y1f         ,"y1"     ,"The y1(x) bessel function");

    ADD_FUNCTION2(float(*)(int,float),jnf,"jn","jn(n,x) bessel function");
    ADD_FUNCTION2(float(*)(int,float),ynf,"yn","yn(n,x) bessel function");

    add_function("sum",     sum,      "sum(arg1,...) the sum of arguments");
    add_function("product", product,  "product(arg1,...) the product of arguments");
    add_function("min",     min,      "min(arg1,...) the min of arguments");
    add_function("max",     max,      "max(arg1,...) the max of arguments");

#ifndef _MSC_VER
    ADD_FUNCTION1(acoshf      ,"acosh"  ,"acosh(x) Hyperbolic arc cosine of X.");
    ADD_FUNCTION1(asinhf      ,"asinh"  ,"asinh(x) Hyperbolic arc sine of X.");
    ADD_FUNCTION1(atanhf      ,"atanh"  ,"atanh(x) Hyperbolic arc tangent of X.");
    ADD_FUNCTION1(expm1f      ,"expm1"  ,"expm1(x) Return exp(X) - 1.");
    ADD_FUNCTION1(cbrtf       ,"cbrt"   ,"cbrt(x) The cube root of X.");
    ADD_FUNCTION1(dremf       ,"drem"   ,"drem(x,y) Return the remainder of X/Y.");
    ADD_FUNCTION1(erff        ,"erf"    ,"erf(x) Error function");
    ADD_FUNCTION1(erfcf       ,"erfc"   ,"The erfc function");
    ADD_FUNCTION1(lgammaf     ,"gamma"  ,"The gamma(x) function");
    ADD_FUNCTION1(tgammaf     ,"tgamma" ,"The true gamma function tgamma(x)");
    ADD_FUNCTION1(exp2f       ,"exp2"   ,"exp2(x) base-2 exponential of X.");
    ADD_FUNCTION1(log2f       ,"log2"   ,"log2(x) base-2 logarithm of X.");
    ADD_FUNCTION1(roundf      ,"round"  ,"round(x) Round X to nearest integral value, rounding halfway cases away from zero.");
    ADD_FUNCTION1(truncf      ,"trunc"  ,"trunc(x) Round X to the integral value in floating-point format nearest but not larger in magnitude.");
    ADD_FUNCTION1(fdimf       ,"fdim"   ,"fdim(x,y) Return positive difference between X and Y.");
    ADD_FUNCTION1(fmaxf       ,"fmax"   ,"fmax(x,y) Return maximum numeric value from X and Y");
    ADD_FUNCTION1(fminf       ,"fmin"   ,"fmin(x,y) Return maximum numeric value from X and Y");
    ADD_FUNCTION1(fmaf        ,"fma"    ,"fma(x,y,z) Multiply-add function computed as a ternary operation.");
    ADD_FUNCTION1(scalbf      ,"scalb"  ,"scalb(x,n) Return X times (2 to the Nth power).");
    ADD_FUNCTION1(log1pf      ,"log1p"  ,"log1p(x) Return log(1 + X).");
    ADD_FUNCTION1(logbf       ,"logb"   ,"The logb(x) base 2 signed integral exponent of X.");
    ADD_FUNCTION1(significandf,"significand","significand(x) Return the fractional part of X after dividing out `ilogb (X)'.");
    ADD_FUNCTION1(copysignf   ,"copysign","copysign(x,y) Return X with its signed changed to Y's.");
    ADD_FUNCTION1(rintf       ,"rint"   ,"rint(x) Return the integer nearest X in the direction of the prevailing rounding mode.");
    ADD_FUNCTION1(nextafterf  ,"nextafter","nextafter(x,y) Return X + epsilon if X < Y, X - epsilon if X > Y.");
    ADD_FUNCTION1(remainderf  ,"remainder","remainder(x,y) Return the remainder of integer division X / Y with infinite precision.");
    ADD_FUNCTION1(nearbyintf  ,"nearbyint","nearbyint(x) Round X to integral value in floating-point format using current rounding direction");
    ADD_FUNCTION2(int(*)(float),isinff,"isinf","isinf(x) Return 0 if VALUE is finite or NaN, +1 if it is +Infinity, -1 if it is -Infinity. ");
    ADD_FUNCTION2(int(*)(float),finitef,"finite","finite(x) Return nonzero if VALUE is finite and not NaN.");
    ADD_FUNCTION2(int(*)(float),isnanf,"isnan","isnan(x) Return nonzero if VALUE is not a number.");
    ADD_FUNCTION2(float(*)(float,int),scalbnf,"scalbn","The scalbn(x, int n ) Return X times (2 to the Nth power).");
    ADD_FUNCTION2(float(*)(float,long),scalblnf,"scalbln","The scalbln(x, long n) Return X times (2 to the Nth power).");
#endif
    }

protected:

  bool parse_number( float * value, const char * curpos, char ** endptr )
    {
#ifdef _MSC_VER
    * value = (float)strtod(curpos,endptr);
#else
    * value = strtof(curpos,endptr);
#endif
    return *endptr>curpos;
    }

  static float sinc(float x)
    { return x?sinf(x)/x:0;
    }

  static float gexp(float x)
    { return expf(-x*x/2);
    }

  static float rand()
    { return (float )::rand()/RAND_MAX;
    }
  };

#endif /* MATHEXPRESSION_H_ */
