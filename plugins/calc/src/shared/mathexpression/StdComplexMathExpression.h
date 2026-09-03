/*
 * StdComplexMathExpression.h
 *
 *  Created on: May 27, 2009
 *      Author: amyznikov
 */

#ifndef __STD_COMPLEX_MATHEXPRESSION_H__
#define __STD_COMPLEX_MATHEXPRESSION_H__

#include <cmath>
#include <cfloat>
#include <complex>
#include <stdexcept>
#include "MathExpressionBase.h"

template<class T>
class StdComplexMathExpression
  : public MathExpressionBase<std::complex<T> >
  {
  typedef StdComplexMathExpression this_class;
  typedef MathExpressionBase<std::complex<T> > base_class;
  using base_class :: BinaryOpTable;

public:

  typedef T value_type;
  typedef std::complex<T> complex_type;
  StdComplexMathExpression()
    {
    BinaryOpTable.set_num_levels(4);
    BinaryOpTable[0]->template add<std::operator==<value_type> > ("==");
    BinaryOpTable[0]->template add<std::operator!=<value_type> >("!=");
    BinaryOpTable[1]->template add<base_class::operator_plus>("+");
    BinaryOpTable[1]->template add<base_class::operator_minus>("-");
    BinaryOpTable[2]->template add<base_class::operator_multiply >("*");
    BinaryOpTable[2]->template add<base_class::operator_divide >("/");
    BinaryOpTable[3]->template add<std::pow<value_type> >("**");

    base_class::template add_function<std::conj<value_type> >("conj","complex conjugate");
    base_class::template add_function<std::exp<value_type> > ("exp","complex exponential function");
    base_class::template add_function<std::cos<value_type> >("cos","complex cosine function");
    base_class::template add_function<std::cosh<value_type> >("cosh","complex hyperbolic cosine");
    base_class::template add_function<std::log<value_type> >("log","natural logarithm of a complex number");
    base_class::template add_function<std::log10<value_type> >("log10","base-10 logarithm of a complex number");
    base_class::template add_function<std::pow<value_type> >("pow","complex power function");
    base_class::template add_function<std::sin<value_type> >("sin","complex sine function");
    base_class::template add_function<std::sinh<value_type> >("sinh","complex hyperbolic sine");
    base_class::template add_function<std::sqrt<value_type> >("sqrt","complex square root");
    base_class::template add_function<std::tan<value_type> >("tan","complex tangent function");
    base_class::template add_function<std::tanh<value_type> >("tanh","complex hyperbolic tangent");

    base_class::template add_function<&this_class::real> ("real","get real part of a complex number");
    base_class::template add_function<&this_class::imag> ("imag","get imaginary part of a complex number");
    base_class::template add_function<&this_class::polar>("polar","Return complex with magnitude @a rho and angle @a theta");
    base_class::template add_function<&this_class::arg >("arg","get argument of a complex number");
    base_class::template add_function<&this_class::abs>("abs","absolute value of a complex number");
    base_class::template add_function<&this_class::norm>("norm","Return @a z magnitude squared");
    base_class::template add_function<&this_class::rand> ("rand","get real random number");
    base_class::template add_function<&this_class::crand> ("crand","get complex random number");

    base_class::template add_function("sum",base_class :: sum,"calculates the sum of arguments");
    base_class::template add_function("product",base_class :: product,"calculates the product of arguments");

    }
protected:
  static complex_type rand()
    { return complex_type((value_type)::rand()/RAND_MAX);
    }
  static complex_type crand()
    { return complex_type((value_type)::rand()/RAND_MAX,(value_type)::rand()/RAND_MAX);
    }
  static complex_type real( const complex_type & x )
    { return x.real();
    }
  static complex_type imag( const complex_type & x )
    { return x.imag();
    }
  static complex_type arg( const complex_type & x )
    { return std::arg(x);
    }
  static complex_type abs( const complex_type & x )
    { return std::abs(x);
    }
  static complex_type norm( const complex_type & x )
    { return std::norm(x);
    }
  static complex_type polar(const complex_type & a, const complex_type & b )
    {
    if( a.imag() != 0 )
      { throw std::invalid_argument("polar() : first argument is not real");
      }
    if( b.imag() != 0 )
      { throw std::invalid_argument("polar() : second argument is not real");
      }
    return std::polar(a.real(),b.real());
    }

protected:
  bool parse_number( complex_type * value, const char * curpos, char ** endptr )
    {
      if( (*curpos == 'i' || *curpos == 'I') && !base_class :: can_be_part_of_identifier(*(curpos+1)) )
        { value->real() = 0, value->imag() = 1;
          *endptr = (char*)( curpos+1 );
          return true;
        }

      value_type v = (value_type)strtod(curpos,endptr);
      if( *endptr > curpos )
        {
        if( **endptr=='i' || **endptr == 'I' )
          { value->real() = 0, value->imag() = v;
            ++*endptr;
          }
        else
          { value->real() = v, value->imag() = 0;
          }
        return true;
        }
      return false;
    }
  };



#endif /* __STD_COMPLEX_MATHEXPRESSION_H__ */
