/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// -*- C++ -*-

#ifndef XM_COMPLEX_N_H_

// $Id: complex_n.h,v 1.4 2005/12/20 15:57:30 rawson Exp $
//
// Complex number classes
// Modified: 09-04 bdil24 DR 505102. added definition of 
//		   complex_8 + real_8 addition operator
//

// ///// Standard include files

#include <iostream>
#include <math.h>


// ///// X-Midas include files

#include "cdefs.h"


// ///// Complex Number definitions

  // Forward declaration of class complex_16

class complex_16;


// The complex_8 class uses real_4 variables to store the real and
// imaginary parts of the complex number.

class complex_8 {

  public:

    // ///// Data Members

    // Although these data members are public for historical reasons,
    // it is more readable and better style to use the real() and imag()
    // inline functions to access them.

    real_4 re;
    real_4 im;


    // ///// Methods

    // Constructors.  Explicit initialization with a complex number, a
    // real and an imaginary part, or just a real part is allowed.
    // For performance reasons, the default constructor doesn't
    // initialize anything.

    complex_8 (const real_4 real_part, const real_4 imag_part = 0.0)
    {
      re = real_part;
      im = imag_part;
    }

    complex_8 () { }

    complex_8 (const complex_8& init)
    {
      re = init.re;
      im = init.im;
    }

    complex_8 (const complex_16& init);


    // Inspectors

    real_4 real () const { return re; }

    real_4 imag () const { return im; }


    // ///// Comparison operators

    bool operator== (const complex_8& rhs) const
    {
      return (this->re == rhs.re) && (this->im == rhs.im);
    }

    bool operator== (const complex_16& rhs) const;

    bool operator!= (const complex_8& rhs) const
    {
      return (this->re != rhs.re) || (this->im != rhs.im);
    }

    bool operator!= (const complex_16& rhs) const;


    // ///// Unary operators

    complex_8 operator- () const
    {
      return complex_8(-this->re, -this->im);
    }

    complex_8 operator+ () const
    {
      return complex_8(this->re, this->im);
    }

    bool operator! () const
    {
      if (((0.0 == this->re) || (-0.0 == this->re)) &&
	  ((0.0 == this->im) || (-0.0 == this->im))) {
	return true;
      }
      return false;
    }

    complex_8 operator~ () const
    {
      return complex_8(this->re, -this->im);
    }


    // ///// Binary operators

    // Assignment

    complex_8& operator= (const complex_8& rhs)
    {
      re = rhs.re; im = rhs.im; return *this;
    }

    complex_8& operator= (const complex_16& rhs);


    // Addition

    complex_8 operator+ (const complex_8& rhs) const
    {
      return complex_8(this->re + rhs.re, this->im + rhs.im);
    }

    complex_8 operator+ (const complex_16& rhs) const;

    complex_8 operator+ (const real_4& rhs) const
    {
      return complex_8(this->re + rhs, this->im);
    }

    complex_16 operator+ (const real_8& rhs) const;


    // Addition assignment

    complex_8& operator+= (const complex_8& rhs)
    {
      this->re += rhs.re; this->im += rhs.im; return *this;
    }

    complex_8& operator+= (const complex_16& rhs);

    complex_8& operator+= (const real_4& rhs)
    {
      this->re += rhs; return *this;
    }


    // Subtraction

    complex_8 operator- (const complex_8& rhs) const
    {
      return complex_8(this->re - rhs.re, this->im - rhs.im);
    }

    complex_8 operator- (const complex_16& rhs) const;

    complex_8 operator- (const real_4& rhs) const
    {
      return complex_8(this->re - rhs, this->im);
    }


    // Subtraction assignment

    complex_8& operator-= (const complex_8& rhs)
    {
      this->re -= rhs.re; this->im -= rhs.im; return *this;
    }

    complex_8& operator-= (const complex_16& rhs);

    complex_8& operator-= (const real_4& rhs)
    {
      this->re -= rhs; return *this;
    }


    // Multiplication

    complex_8 operator* (const complex_8& rhs) const
    {
      return complex_8(this->re * rhs.re - this->im * rhs.im,
		       this->im * rhs.re + this->re * rhs.im);
    }

    complex_8 operator* (const complex_16& rhs) const;

    complex_8 operator* (const real_4& rhs) const
    {
      return complex_8(this->re * rhs, this->im * rhs);
    }


    // Multiplication assignment

    complex_8& operator*= (const complex_8& rhs)
    {
      real_4 temp_re = this->re * rhs.re - this->im * rhs.im;
      this->im = this->im * rhs.re + this->re * rhs.im;
      this->re = temp_re;
      return *this;
    }

    complex_8& operator*= (const complex_16& rhs);

    complex_8& operator*= (const real_4& rhs)
    {
      this->re *= rhs; this->im *= rhs; return *this;
    }


    // Division

    complex_8 operator/ (const complex_8& rhs) const
    {
      real_8 denom = real_8(rhs.re) * rhs.re + real_8(rhs.im) * rhs.im;
      return complex_8((real_8(this->re) * rhs.re +
			real_8(this->im) * rhs.im) / denom,
		       (real_8(this->im) * rhs.re -
			real_8(this->re) * rhs.im) / denom);
    }

    complex_8 operator/ (const complex_16& rhs) const;

    complex_8 operator/ (const real_4& rhs) const
    {
      return complex_8(this->re / rhs, this->im / rhs);
    }


    // Division assignment

    complex_8& operator/= (const complex_8& rhs)
    {
      real_8 denom = real_8(rhs.re) * rhs.re + real_8(rhs.im) * rhs.im;
      real_4 temp_re = (real_8(this->re) * rhs.re +
			real_8(this->im) * rhs.im) / denom;
      this->im = (real_8(this->im) * rhs.re -
		  real_8(this->re) * rhs.im) / denom;
      this->re = temp_re;
      return *this;
    }

    complex_8& operator/= (const complex_16& rhs);

    complex_8& operator/= (const real_4& rhs)
    {
      this->re /= rhs; this->im /= rhs; return *this;
    }

};


// Global functions that operate on a complex_8 number

inline complex_8 conj (const complex_8& c) {
  return complex_8(c.re, -c.im);
}

inline complex_8 polar (real_4 rho, real_4 theta)
{
  return complex_8(rho * cos(theta), rho * sin(theta));
}


// Output operator for the complex_8 class.  Note that this function
// does not need to be a friend of the class, since the data members
// are public.

inline std::ostream& operator<< (std::ostream& output, const complex_8& c)
{
  output << "(" << c.re << ", " << c.im << ")";
  return output;
}


// Operators that have a built-in type on the left and a complex_8 on
// the right

// Addition

inline complex_8 operator+ (const real_4& lhs, const complex_8& rhs)
{
  return complex_8(rhs.re + lhs, rhs.im);
}


// Subtraction

inline complex_8 operator- (const real_4& lhs, const complex_8& rhs)
{
  return complex_8(lhs - rhs.re, -rhs.im);
}


// Multiplication

inline complex_8 operator* (const real_4& lhs, const complex_8& rhs)
{
  return complex_8(rhs.re * lhs, rhs.im * lhs);
}


// Division

inline complex_8 operator/ (const real_8& lhs, const complex_8& rhs)
{
  real_8 denom = real_8(rhs.re) * rhs.re + real_8(rhs.im) * rhs.im;
  return complex_8(rhs.re * lhs / denom, -rhs.im * lhs / denom);
}


// The complex_16 class uses real_8 variables to store the real and
// imaginary parts of the complex number.

class complex_16 {

  public:

    // ///// Data Members

    real_8 re;
    real_8 im;


    // ///// Methods

    // Constructors.  Explicit initialization with a complex number, a
    // real and an imaginary part, or just a real part is allowed.
    // For performance reasons, the default constructor doesn't
    // initialize anything.

    complex_16 (real_8 real_part, real_8 imag_part = 0.0) {
      re = real_part;
      im = imag_part;
    }

    complex_16 () { }

    complex_16 (const complex_16& init)
    {
      re = init.re;
      im = init.im;
    }

    complex_16 (const complex_8& init)
    {
      re = real_8(init.re);
      im = real_8(init.im);
    }


    // ///// Inspectors

    real_8 real () const { return re; }

    real_8 imag () const { return im; }


    // ///// Comparison operators

    bool operator== (const complex_16& rhs) const
    {
      return (this->re == rhs.re) && (this->im == rhs.im);
    }

    bool operator== (const complex_8& rhs) const
    {
      return (this->re == real_8(rhs.re)) && (this->im == real_8(rhs.im));
    }

    bool operator!= (const complex_16& rhs) const
    {
      return (this->re != rhs.re) || (this->im != rhs.im);
    }

    bool operator!= (const complex_8& rhs) const
    {
      return (this->re != real_8(rhs.re)) || (this->im != real_8(rhs.im));
    }


    // ///// Unary operators

    complex_16 operator- () const
    {
      return complex_16(-this->re, -this->im);
    }

    complex_16 operator+ () const
    {
      return complex_16(this->re, this->im);
    }

    bool operator! () const
    {
      if (((0.0 == this->re) || (-0.0 == this->re)) &&
	  ((0.0 == this->im) || (-0.0 == this->im))) {
	return true;
      }
      return false;
    }

    complex_16 operator~ () const
    {
      return complex_16(this->re, -this->im);
    }


    // ///// Binary operators

    // Assignment

    complex_16& operator= (const complex_16& rhs)
    {
      re = rhs.re; im = rhs.im; return *this;
    }

    complex_16& operator= (const complex_8& rhs)
    {
      re = real_8(rhs.re); im = real_8(rhs.im); return *this;
    }


    // Addition

    complex_16 operator+ (const complex_16& rhs) const
    {
      return complex_16(this->re + rhs.re, this->im + rhs.im);
    }

    complex_16 operator+ (const complex_8& rhs) const
    {
      return complex_16(this->re + real_8(rhs.re), this->im + real_8(rhs.im));
    }

    complex_16 operator+ (const real_8& rhs) const
    {
      return complex_16(this->re + rhs, this->im);
    }


    // Addition assignment

    complex_16& operator+= (const complex_16& rhs)
    {
      this->re += rhs.re; this->im += rhs.im; return *this;
    }

    complex_16& operator+= (const complex_8& rhs)
    {
      this->re += real_8(rhs.re); this->im += real_8(rhs.im); return *this;
    }

    complex_16& operator+= (const real_8& rhs)
    {
      this->re += rhs; return *this;
    }


    // Subtraction

    complex_16 operator- (const complex_16& rhs) const
    {
      return complex_16(this->re - rhs.re, this->im - rhs.im);
    }

    complex_16 operator- (const complex_8& rhs) const
    {
      return complex_16(this->re - real_8(rhs.re), this->im - real_8(rhs.im));
    }

    complex_16 operator- (const real_8& rhs) const
    {
      return complex_16(this->re - rhs, this->im);
    }


    // Subtraction assignment

    complex_16& operator-= (const complex_16& rhs)
    {
      this->re -= rhs.re; this->im -= rhs.im; return *this;
    }

    complex_16& operator-= (const complex_8& rhs)
    {
      this->re -= real_8(rhs.re); this->im -= real_8(rhs.im); return *this;
    }

    complex_16& operator-= (const real_8& rhs)
    {
      this->re -= rhs; return *this;
    }


    // Multiplication

    complex_16 operator* (const complex_16& rhs) const
    {
      return complex_16(this->re * rhs.re - this->im * rhs.im,
			this->im * rhs.re + this->re * rhs.im);
    }

    complex_16 operator* (const complex_8& rhs) const
    {
      return complex_16(this->re * rhs.re - this->im * rhs.im,
			this->im * rhs.re + this->re * rhs.im);
    }

    complex_16 operator* (const real_8& rhs) const
    {
      return complex_16(this->re * rhs, this->im * rhs);
    }


    // Multiplication

    complex_16& operator*= (const complex_16& rhs)
    {
      real_8 temp_re = this->re * rhs.re - this->im * rhs.im;
      this->im = this->im * rhs.re + this->re * rhs.im;
      this->re = temp_re;
      return *this;
    }

    complex_16& operator*= (const complex_8& rhs)
    {
      real_8 temp_re = this->re * rhs.re - this->im * rhs.im;
      this->im = this->im * rhs.re + this->re * rhs.im;
      this->re = temp_re;
      return *this;
    }

    complex_16& operator*= (const real_8& rhs)
    {
      this->re *= rhs; this->im *= rhs; return *this;
    }


    // Division

    complex_16 operator/ (const complex_16& rhs) const
    {
      real_8 denom = rhs.re * rhs.re + rhs.im * rhs.im;
      return complex_16((this->re * rhs.re + this->im * rhs.im) / denom,
			(this->im * rhs.re - this->re * rhs.im) / denom);
    }

    complex_16 operator/ (const complex_8& rhs) const
    {
      real_8 denom = rhs.re * rhs.re + rhs.im * rhs.im;
      return complex_16((this->re * rhs.re + this->im * rhs.im) / denom,
			(this->im * rhs.re - this->re * rhs.im) / denom);
    }

    complex_16 operator/ (const real_8& rhs) const
    {
      return complex_16(this->re / rhs, this->im / rhs);
    }


    // Division assignment

    complex_16& operator/= (const complex_16& rhs)
    {
      real_8 denom = rhs.re * rhs.re + rhs.im * rhs.im;
      real_8 temp_re = (this->re * rhs.re + this->im * rhs.im) / denom;
      this->im = (this->im * rhs.re - this->re * rhs.im) / denom;
      this->re = temp_re;
      return *this;
    }

    complex_16& operator/= (const complex_8& rhs)
    {
      real_8 denom = rhs.re * rhs.re + rhs.im * rhs.im;
      real_8 temp_re = (this->re * rhs.re + this->im * rhs.im) / denom;
      this->im = (this->im * rhs.re - this->re * rhs.im) / denom;
      this->re = temp_re;
      return *this;
    }

    complex_16& operator/= (const real_8& rhs)
    {
      this->re /= rhs; this->im /= rhs; return *this;
    }

};


// Global functions that operate on a complex_16 number

inline complex_16 conj (const complex_16& c) {
  return complex_16(c.re, -c.im);
}

inline complex_16 polar (real_8 rho, real_8 theta)
{
  return complex_16(rho * cos(theta), rho * sin(theta));
}


// Output operator for the complex_16 class.  Note that this function
// does not need to be a friend of the class, since the data members
// are public.

inline std::ostream& operator<< (std::ostream& output, const complex_16& c)
{
  output << "(" << c.re << ", " << c.im << ")";
  return output;
}


// Operators that have a built-in type on the left and a complex_16 on
// the right

// Addition

inline complex_16 operator+ (const real_8& lhs, const complex_16& rhs)
{
  return complex_16(rhs.re + lhs, rhs.im);
}


// Subtraction

inline complex_16 operator- (const real_8& lhs, const complex_16& rhs)
{
  return complex_16(lhs - rhs.re, -rhs.im);
}


// Multiplication

inline complex_16 operator* (const real_8& lhs, const complex_16& rhs)
{
  return complex_16(rhs.re * lhs, rhs.im * lhs);
}


// Division

inline complex_16 operator/ (const real_8& lhs, const complex_16& rhs)
{
  real_8 denom = rhs.re * rhs.re + rhs.im * rhs.im;
  return complex_16(rhs.re * lhs / denom, -rhs.im * lhs / denom);
}


// Definitions for complex_8 that had to wait for definition of complex_16

inline complex_8::complex_8 (const complex_16& init)
{
  re = real_4(init.re);
  im = real_4(init.im);
}

inline bool complex_8::operator== (const complex_16& rhs) const
{
  return (this->re == real_4(rhs.re)) && (this->im == real_4(rhs.im));
}

inline bool complex_8::operator!= (const complex_16& rhs) const
{
  return (this->re != real_4(rhs.re)) || (this->im != real_4(rhs.im));
}

inline complex_8& complex_8::operator= (const complex_16& rhs)
{
  re = real_4(rhs.re); im = real_4(rhs.im); return *this;
}

inline complex_8 complex_8::operator+ (const complex_16& rhs) const
{
  return complex_8(this->re + real_4(rhs.re), this->im + real_4(rhs.im));
}

inline complex_16 complex_8::operator+ (const real_8&rhs) const
{
  return complex_16(this->re + rhs, this->im);
}

inline complex_8& complex_8::operator+= (const complex_16& rhs)
{
  this->re += real_4(rhs.re);  this->im += real_4(rhs.im); return *this;
}

inline complex_8 complex_8::operator- (const complex_16& rhs) const
{
  return complex_8(this->re - real_4(rhs.re), this->im - real_4(rhs.im));
}

inline complex_8& complex_8::operator-= (const complex_16& rhs)
{
  this->re -= real_4(rhs.re);  this->im -= real_4(rhs.im); return *this;
}

inline complex_8 complex_8::operator* (const complex_16& rhs) const
{
  return complex_8(real_4(this->re * rhs.re - this->im * rhs.im),
		   real_4(this->im * rhs.re + this->re * rhs.im));
}

inline complex_8& complex_8::operator*= (const complex_16& rhs)
{
  real_8 temp_re = this->re * rhs.re - this->im * rhs.im;
  this->im = real_4(this->im * rhs.re + this->re * rhs.im);
  this->re = real_4(temp_re);
  return *this;
}

inline complex_8 complex_8::operator/ (const complex_16& rhs) const
{
  real_8 denom = rhs.re * rhs.re + rhs.im * rhs.im;
  return complex_8(real_4((this->re * rhs.re + this->im * rhs.im) / denom),
		   real_4((this->im * rhs.re - this->re * rhs.im) / denom));
}

inline complex_8& complex_8::operator/= (const complex_16& rhs)
{
  real_8 denom = rhs.re * rhs.re + rhs.im * rhs.im;
  real_4 temp_re = real_4((this->re * rhs.re + this->im * rhs.im) / denom);
  this->im = real_4((this->im * rhs.re - this->re * rhs.im) / denom);
  this->re = temp_re;
  return *this;
}


// Real and imaginary part inspectors

inline real_4 real (const complex_8& cx) { return cx.re; }
inline real_4 imag  (const complex_8& cx) { return cx.im; }
inline real_4 aimag (const complex_8& cx) { return cx.im; }

inline real_8 real (const complex_16& cx) { return cx.re; }
inline real_8 imag  (const complex_16& cx) { return cx.im; }
inline real_8 aimag (const complex_16& cx) { return cx.im; }


// Absolute value functions

inline real_4 abs (const complex_8& cx)
{
  return sqrtf(cx.re * cx.re + cx.im * cx.im);
}

inline real_4 abs2 (const complex_8& cx)
{
  return cx.re * cx.re + cx.im * cx.im;
}

inline real_4 norm (const complex_8& cx)
{
  return abs2(cx);
}			// comply with std::complex

inline real_8 abs (const complex_16& cx)
{
  return sqrt(cx.re * cx.re + cx.im * cx.im);
}

inline real_8 abs2 (const complex_16& cx)
{
  return cx.re * cx.re + cx.im * cx.im;
}

inline real_8 norm (const complex_16& cx)
{
  return abs2(cx);
}			// comply with std::complex



// //////////////////////////////////////////////////////////////////
//			The Complex Higher Math Functions
// //////////////////////////////////////////////////////////////////

// Predeclared (because of mutual dependencies)
inline real_8 phase (const complex_16& cx);


// ///// Exponentiation

inline complex_16 exp (const complex_16& cx)
{
  return complex_16(cos(cx.im), sin(cx.im))*exp(cx.re);
}

inline complex_16 log (const complex_16& cx)
{
  return complex_16(log(abs(cx)), phase(cx));
}

inline complex_16 pow (const complex_16& cx, const complex_16& cexp)
{
  return exp( cexp*log(cx) );
}

inline complex_16 pow (const complex_16& cx, const real_8& exponent)
{
  return exp(log(cx)*exponent);
}

inline complex_16 pow (const real_8& val, const complex_16& cexp)
{
  return exp(cexp*log(val));
}

inline complex_16 log10 (const complex_16& cx)
{
  return log(cx) / log(10.0);
}

inline complex_16 sqrt (const complex_16& cx)
{
  // 08/99:  pow() eventually causes all call to log(abs(cx)) 
  // and also phase(cx).  If cx=(0,0) the result is machine
  // dependent and may be (-inf, NaNQ)...which will cause
  // an exception when this value is used in later calculations.
  if (abs(cx) == 0) 
    return complex_16(0,0);
  else
    return pow(cx, 0.5);

  
}


// ///// Trigonometric Functions

inline real_8 phase (const complex_16& cx)
{
  return atan2(cx.im, cx.re);
}					// phase of a complex number

inline real_8 arg (const complex_16& cx)
{
  return phase(cx);
}					// comply with std::complex

inline complex_16 sin (const complex_16& cx)
{
  return complex_16(sin(cx.re) * cosh(cx.im), cos(cx.re) * sinh(cx.im));
}					// sin of a complex number

inline complex_16 cos (const complex_16& cx)
{
  return complex_16(cos(cx.re) * cosh(cx.im),  - sin(cx.re) * sinh(cx.im));
}					// cos of a complex number

inline complex_16 tan (const complex_16& cx)
{
  return sin(cx)/cos(cx);
}					// tan of a complex number

inline complex_16 asin (const complex_16& cx)
{
  // asin(x) = -i * ln(ix + sqrt(1 - x**2))
  complex_16 icx(-cx.im, cx.re);  // multiplying cx by i
  complex_16 temp = log(icx + sqrt(1 - cx*cx));
  return complex_16(temp.im, -temp.re); // multiplying by -i
}					// asin of a complex number

inline complex_16 acos (const complex_16& cx)
{
  // acos(x) = -i * ln(x + i * sqrt(1 - x**2))
  complex_16 temp(sqrt(1 - cx*cx));
  temp = log(cx + complex_16(-temp.im, temp.re));
  return complex_16(temp.im, -temp.re); // multiplying by -i
}					// acos of a complex number

inline complex_16 atan (const complex_16& cx)
{
  // atan(x) = -(i/2) * ln((1 + ix)/(1 - ix))
  complex_16 icx(-cx.im, cx.re);
  complex_16 temp = log((1 + icx)/(1 - icx));
  return -0.5 * complex_16(-temp.im, temp.re); // multiplying by i
}					// atan of a complex number


// ///// Hyperbolic Functions

inline complex_16 sinh (const complex_16& cx)
{
  return complex_16(sinh(cx.re) * cos(cx.im), cosh(cx.re) * sin(cx.im));
}					// sinh of a complex number

inline complex_16 cosh(const complex_16& cx)
{
  return complex_16(cosh(cx.re) * cos(cx.im),  sinh(cx.re) * sin(cx.im));
}					// cosh of a complex number

inline complex_16 tanh(const complex_16& cx)
{
  return sinh(cx)/cosh(cx);
}					// tanh of a complex number

inline complex_16 asinh (const complex_16& cx)
{
  // asinh(x) = ln(x + sqrt(x**2 + 1))
  return log(cx + sqrt(cx*cx + 1));
}					// asinh of a complex number

inline complex_16 acosh (const complex_16& cx)
{
  // acosh(x) = ln(x + sqrt(x**2 - 1))
  return log(cx + sqrt(cx*cx - 1));
}					// acos of a complex number

inline complex_16 atanh (const complex_16& cx)
{
  // atan(x) = (1/2) * ln((1 + ix)/(1 - ix))
  return 0.5 * log((1 + cx)/(1 - cx));
}					// atan of a complex number



#define XM_COMPLEX_N_H_
#endif  // XM_COMPLEX_N_H_
