/** @file power.cpp
 *
 *  Implementation of GiNaC's symbolic exponentiation (basis^exponent). */

/*
 *  GiNaC Copyright (C) 1999-2002 Johannes Gutenberg University Mainz, Germany
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <vector>
#include <iostream>
#include <stdexcept>

#include "power.h"
#include "expairseq.h"
#include "add.h"
#include "mul.h"
#include "ncmul.h"
#include "numeric.h"
#include "constant.h"
#include "operators.h"
#include "inifcns.h" // for log() in power::derivative()
#include "matrix.h"
#include "indexed.h"
#include "symbol.h"
#include "print.h"
#include "archive.h"
#include "utils.h"

namespace GiNaC {

GINAC_IMPLEMENT_REGISTERED_CLASS(power, basic)

typedef std::vector<int> intvector;

//////////
// default ctor, dtor, copy ctor, assignment operator and helpers
//////////

power::power() : inherited(TINFO_power) { }

void power::copy(const power & other)
{
	inherited::copy(other);
	basis = other.basis;
	exponent = other.exponent;
}

DEFAULT_DESTROY(power)

//////////
// other ctors
//////////

// all inlined

//////////
// archiving
//////////

power::power(const archive_node &n, const lst &sym_lst) : inherited(n, sym_lst)
{
	n.find_ex("basis", basis, sym_lst);
	n.find_ex("exponent", exponent, sym_lst);
}

void power::archive(archive_node &n) const
{
	inherited::archive(n);
	n.add_ex("basis", basis);
	n.add_ex("exponent", exponent);
}

DEFAULT_UNARCHIVE(power)

//////////
// functions overriding virtual functions from base classes
//////////

// public

static void print_sym_pow(const print_context & c, const symbol &x, int exp)
{
	// Optimal output of integer powers of symbols to aid compiler CSE.
	// C.f. ISO/IEC 14882:1998, section 1.9 [intro execution], paragraph 15
	// to learn why such a parenthisation is really necessary.
	if (exp == 1) {
		x.print(c);
	} else if (exp == 2) {
		x.print(c);
		c.s << "*";
		x.print(c);
	} else if (exp & 1) {
		x.print(c);
		c.s << "*";
		print_sym_pow(c, x, exp-1);
	} else {
		c.s << "(";
		print_sym_pow(c, x, exp >> 1);
		c.s << ")*(";
		print_sym_pow(c, x, exp >> 1);
		c.s << ")";
	}
}

void power::print(const print_context & c, unsigned level) const
{
	if (is_a<print_tree>(c)) {

		inherited::print(c, level);

	} else if (is_a<print_csrc>(c)) {

		// Integer powers of symbols are printed in a special, optimized way
		if (exponent.info(info_flags::integer)
		 && (is_exactly_a<symbol>(basis) || is_exactly_a<constant>(basis))) {
			int exp = ex_to<numeric>(exponent).to_int();
			if (exp > 0)
				c.s << '(';
			else {
				exp = -exp;
				if (is_a<print_csrc_cl_N>(c))
					c.s << "recip(";
				else
					c.s << "1.0/(";
			}
			print_sym_pow(c, ex_to<symbol>(basis), exp);
			c.s << ')';

		// <expr>^-1 is printed as "1.0/<expr>" or with the recip() function of CLN
		} else if (exponent.is_equal(_ex_1)) {
			if (is_a<print_csrc_cl_N>(c))
				c.s << "recip(";
			else
				c.s << "1.0/(";
			basis.print(c);
			c.s << ')';

		// Otherwise, use the pow() or expt() (CLN) functions
		} else {
			if (is_a<print_csrc_cl_N>(c))
				c.s << "expt(";
			else
				c.s << "pow(";
			basis.print(c);
			c.s << ',';
			exponent.print(c);
			c.s << ')';
		}

	} else if (is_a<print_python_repr>(c)) {

		c.s << class_name() << '(';
		basis.print(c);
		c.s << ',';
		exponent.print(c);
		c.s << ')';

	} else {

		bool is_tex = is_a<print_latex>(c);

		if (exponent.is_equal(_ex1_2)) {
			c.s << (is_tex ? "\\sqrt{" : "sqrt(");
			basis.print(c);
			c.s << (is_tex ? '}' : ')');
		} else {
			if (precedence() <= level)
				c.s << (is_tex ? "{(" : "(");
			basis.print(c, precedence());
			if (is_a<print_python>(c))
				c.s << "**";
			else
				c.s << '^';
			if (is_tex)
				c.s << '{';
			exponent.print(c, precedence());
			if (is_tex)
				c.s << '}';
			if (precedence() <= level)
				c.s << (is_tex ? ")}" : ")");
		}
	}
}

bool power::info(unsigned inf) const
{
	switch (inf) {
		case info_flags::polynomial:
		case info_flags::integer_polynomial:
		case info_flags::cinteger_polynomial:
		case info_flags::rational_polynomial:
		case info_flags::crational_polynomial:
			return exponent.info(info_flags::nonnegint);
		case info_flags::rational_function:
			return exponent.info(info_flags::integer);
		case info_flags::algebraic:
			return (!exponent.info(info_flags::integer) ||
					basis.info(inf));
	}
	return inherited::info(inf);
}

unsigned power::nops() const
{
	return 2;
}

ex & power::let_op(int i)
{
	GINAC_ASSERT(i>=0);
	GINAC_ASSERT(i<2);

	return i==0 ? basis : exponent;
}

ex power::map(map_function & f) const
{
	return (new power(f(basis), f(exponent)))->setflag(status_flags::dynallocated);
}

int power::degree(const ex & s) const
{
	if (is_equal(ex_to<basic>(s)))
		return 1;
	else if (is_exactly_a<numeric>(exponent) && ex_to<numeric>(exponent).is_integer()) {
		if (basis.is_equal(s))
			return ex_to<numeric>(exponent).to_int();
		else
			return basis.degree(s) * ex_to<numeric>(exponent).to_int();
	} else if (basis.has(s))
		throw(std::runtime_error("power::degree(): undefined degree because of non-integer exponent"));
	else
		return 0;
}

int power::ldegree(const ex & s) const 
{
	if (is_equal(ex_to<basic>(s)))
		return 1;
	else if (is_exactly_a<numeric>(exponent) && ex_to<numeric>(exponent).is_integer()) {
		if (basis.is_equal(s))
			return ex_to<numeric>(exponent).to_int();
		else
			return basis.ldegree(s) * ex_to<numeric>(exponent).to_int();
	} else if (basis.has(s))
		throw(std::runtime_error("power::ldegree(): undefined degree because of non-integer exponent"));
	else
		return 0;
}

ex power::coeff(const ex & s, int n) const
{
	if (is_equal(ex_to<basic>(s)))
		return n==1 ? _ex1 : _ex0;
	else if (!basis.is_equal(s)) {
		// basis not equal to s
		if (n == 0)
			return *this;
		else
			return _ex0;
	} else {
		// basis equal to s
		if (is_exactly_a<numeric>(exponent) && ex_to<numeric>(exponent).is_integer()) {
			// integer exponent
			int int_exp = ex_to<numeric>(exponent).to_int();
			if (n == int_exp)
				return _ex1;
			else
				return _ex0;
		} else {
			// non-integer exponents are treated as zero
			if (n == 0)
				return *this;
			else
				return _ex0;
		}
	}
}

/** Perform automatic term rewriting rules in this class.  In the following
 *  x, x1, x2,... stand for a symbolic variables of type ex and c, c1, c2...
 *  stand for such expressions that contain a plain number.
 *  - ^(x,0) -> 1  (also handles ^(0,0))
 *  - ^(x,1) -> x
 *  - ^(0,c) -> 0 or exception  (depending on the real part of c)
 *  - ^(1,x) -> 1
 *  - ^(c1,c2) -> *(c1^n,c1^(c2-n))  (so that 0<(c2-n)<1, try to evaluate roots, possibly in numerator and denominator of c1)
 *  - ^(^(x,c1),c2) -> ^(x,c1*c2)  (c2 integer or -1 < c1 <= 1, case c1=1 should not happen, see below!)
 *  - ^(*(x,y,z),c) -> *(x^c,y^c,z^c)  (if c integer)
 *  - ^(*(x,c1),c2) -> ^(x,c2)*c1^c2  (c1>0)
 *  - ^(*(x,c1),c2) -> ^(-x,c2)*c1^c2  (c1<0)
 *
 *  @param level cut-off in recursive evaluation */
ex power::eval(int level) const
{
	if ((level==1) && (flags & status_flags::evaluated))
		return *this;
	else if (level == -max_recursion_level)
		throw(std::runtime_error("max recursion level reached"));
	
	const ex & ebasis    = level==1 ? basis    : basis.eval(level-1);
	const ex & eexponent = level==1 ? exponent : exponent.eval(level-1);
	
	bool basis_is_numerical = false;
	bool exponent_is_numerical = false;
	const numeric *num_basis;
	const numeric *num_exponent;
	
	if (is_exactly_a<numeric>(ebasis)) {
		basis_is_numerical = true;
		num_basis = &ex_to<numeric>(ebasis);
	}
	if (is_exactly_a<numeric>(eexponent)) {
		exponent_is_numerical = true;
		num_exponent = &ex_to<numeric>(eexponent);
	}
	
	// ^(x,0) -> 1  (0^0 also handled here)
	if (eexponent.is_zero()) {
		if (ebasis.is_zero())
			throw (std::domain_error("power::eval(): pow(0,0) is undefined"));
		else
			return _ex1;
	}
	
	// ^(x,1) -> x
	if (eexponent.is_equal(_ex1))
		return ebasis;

	// ^(0,c1) -> 0 or exception  (depending on real value of c1)
	if (ebasis.is_zero() && exponent_is_numerical) {
		if ((num_exponent->real()).is_zero())
			throw (std::domain_error("power::eval(): pow(0,I) is undefined"));
		else if ((num_exponent->real()).is_negative())
			throw (pole_error("power::eval(): division by zero",1));
		else
			return _ex0;
	}

	// ^(1,x) -> 1
	if (ebasis.is_equal(_ex1))
		return _ex1;

	if (exponent_is_numerical) {

		// ^(c1,c2) -> c1^c2  (c1, c2 numeric(),
		// except if c1,c2 are rational, but c1^c2 is not)
		if (basis_is_numerical) {
			const bool basis_is_crational = num_basis->is_crational();
			const bool exponent_is_crational = num_exponent->is_crational();
			if (!basis_is_crational || !exponent_is_crational) {
				// return a plain float
				return (new numeric(num_basis->power(*num_exponent)))->setflag(status_flags::dynallocated |
				                                                               status_flags::evaluated |
				                                                               status_flags::expanded);
			}

			const numeric res = num_basis->power(*num_exponent);
			if (res.is_crational()) {
				return res;
			}
			GINAC_ASSERT(!num_exponent->is_integer());  // has been handled by now

			// ^(c1,n/m) -> *(c1^q,c1^(n/m-q)), 0<(n/m-q)<1, q integer
			if (basis_is_crational && exponent_is_crational
			    && num_exponent->is_real()
			    && !num_exponent->is_integer()) {
				const numeric n = num_exponent->numer();
				const numeric m = num_exponent->denom();
				numeric r;
				numeric q = iquo(n, m, r);
				if (r.is_negative()) {
					r += m;
					--q;
				}
				if (q.is_zero()) {  // the exponent was in the allowed range 0<(n/m)<1
					if (num_basis->is_rational() && !num_basis->is_integer()) {
						// try it for numerator and denominator separately, in order to
						// partially simplify things like (5/8)^(1/3) -> 1/2*5^(1/3)
						const numeric bnum = num_basis->numer();
						const numeric bden = num_basis->denom();
						const numeric res_bnum = bnum.power(*num_exponent);
						const numeric res_bden = bden.power(*num_exponent);
						if (res_bnum.is_integer())
							return (new mul(power(bden,-*num_exponent),res_bnum))->setflag(status_flags::dynallocated | status_flags::evaluated);
						if (res_bden.is_integer())
							return (new mul(power(bnum,*num_exponent),res_bden.inverse()))->setflag(status_flags::dynallocated | status_flags::evaluated);
					}
					return this->hold();
				} else {
					// assemble resulting product, but allowing for a re-evaluation,
					// because otherwise we'll end up with something like
					//    (7/8)^(4/3)  ->  7/8*(1/2*7^(1/3))
					// instead of 7/16*7^(1/3).
					ex prod = power(*num_basis,r.div(m));
					return prod*power(*num_basis,q);
				}
			}
		}
	
		// ^(^(x,c1),c2) -> ^(x,c1*c2)
		// (c1, c2 numeric(), c2 integer or -1 < c1 <= 1,
		// case c1==1 should not happen, see below!)
		if (is_exactly_a<power>(ebasis)) {
			const power & sub_power = ex_to<power>(ebasis);
			const ex & sub_basis = sub_power.basis;
			const ex & sub_exponent = sub_power.exponent;
			if (is_exactly_a<numeric>(sub_exponent)) {
				const numeric & num_sub_exponent = ex_to<numeric>(sub_exponent);
				GINAC_ASSERT(num_sub_exponent!=numeric(1));
				if (num_exponent->is_integer() || (abs(num_sub_exponent) - _num1).is_negative())
					return power(sub_basis,num_sub_exponent.mul(*num_exponent));
			}
		}
	
		// ^(*(x,y,z),c1) -> *(x^c1,y^c1,z^c1) (c1 integer)
		if (num_exponent->is_integer() && is_exactly_a<mul>(ebasis)) {
			return expand_mul(ex_to<mul>(ebasis), *num_exponent);
		}
	
		// ^(*(...,x;c1),c2) -> *(^(*(...,x;1),c2),c1^c2)  (c1, c2 numeric(), c1>0)
		// ^(*(...,x;c1),c2) -> *(^(*(...,x;-1),c2),(-c1)^c2)  (c1, c2 numeric(), c1<0)
		if (is_exactly_a<mul>(ebasis)) {
			GINAC_ASSERT(!num_exponent->is_integer()); // should have been handled above
			const mul & mulref = ex_to<mul>(ebasis);
			if (!mulref.overall_coeff.is_equal(_ex1)) {
				const numeric & num_coeff = ex_to<numeric>(mulref.overall_coeff);
				if (num_coeff.is_real()) {
					if (num_coeff.is_positive()) {
						mul *mulp = new mul(mulref);
						mulp->overall_coeff = _ex1;
						mulp->clearflag(status_flags::evaluated);
						mulp->clearflag(status_flags::hash_calculated);
						return (new mul(power(*mulp,exponent),
						                power(num_coeff,*num_exponent)))->setflag(status_flags::dynallocated);
					} else {
						GINAC_ASSERT(num_coeff.compare(_num0)<0);
						if (!num_coeff.is_equal(_num_1)) {
							mul *mulp = new mul(mulref);
							mulp->overall_coeff = _ex_1;
							mulp->clearflag(status_flags::evaluated);
							mulp->clearflag(status_flags::hash_calculated);
							return (new mul(power(*mulp,exponent),
							                power(abs(num_coeff),*num_exponent)))->setflag(status_flags::dynallocated);
						}
					}
				}
			}
		}

		// ^(nc,c1) -> ncmul(nc,nc,...) (c1 positive integer, unless nc is a matrix)
		if (num_exponent->is_pos_integer() &&
		    ebasis.return_type() != return_types::commutative &&
		    !is_a<matrix>(ebasis)) {
			return ncmul(exvector(num_exponent->to_int(), ebasis), true);
		}
	}
	
	if (are_ex_trivially_equal(ebasis,basis) &&
	    are_ex_trivially_equal(eexponent,exponent)) {
		return this->hold();
	}
	return (new power(ebasis, eexponent))->setflag(status_flags::dynallocated |
	                                               status_flags::evaluated);
}

ex power::evalf(int level) const
{
	ex ebasis;
	ex eexponent;
	
	if (level==1) {
		ebasis = basis;
		eexponent = exponent;
	} else if (level == -max_recursion_level) {
		throw(std::runtime_error("max recursion level reached"));
	} else {
		ebasis = basis.evalf(level-1);
		if (!is_exactly_a<numeric>(exponent))
			eexponent = exponent.evalf(level-1);
		else
			eexponent = exponent;
	}

	return power(ebasis,eexponent);
}

ex power::evalm(void) const
{
	const ex ebasis = basis.evalm();
	const ex eexponent = exponent.evalm();
	if (is_a<matrix>(ebasis)) {
		if (is_a<numeric>(eexponent)) {
			return (new matrix(ex_to<matrix>(ebasis).pow(eexponent)))->setflag(status_flags::dynallocated);
		}
	}
	return (new power(ebasis, eexponent))->setflag(status_flags::dynallocated);
}

ex power::subs(const lst & ls, const lst & lr, bool no_pattern) const
{
	const ex &subsed_basis = basis.subs(ls, lr, no_pattern);
	const ex &subsed_exponent = exponent.subs(ls, lr, no_pattern);

	if (are_ex_trivially_equal(basis, subsed_basis)
	 && are_ex_trivially_equal(exponent, subsed_exponent))
		return basic::subs(ls, lr, no_pattern);
	else
		return power(subsed_basis, subsed_exponent).basic::subs(ls, lr, no_pattern);
}

ex power::simplify_ncmul(const exvector & v) const
{
	return inherited::simplify_ncmul(v);
}

// protected

/** Implementation of ex::diff() for a power.
 *  @see ex::diff */
ex power::derivative(const symbol & s) const
{
	if (exponent.info(info_flags::real)) {
		// D(b^r) = r * b^(r-1) * D(b) (faster than the formula below)
		epvector newseq;
		newseq.reserve(2);
		newseq.push_back(expair(basis, exponent - _ex1));
		newseq.push_back(expair(basis.diff(s), _ex1));
		return mul(newseq, exponent);
	} else {
		// D(b^e) = b^e * (D(e)*ln(b) + e*D(b)/b)
		return mul(*this,
		           add(mul(exponent.diff(s), log(basis)),
		           mul(mul(exponent, basis.diff(s)), power(basis, _ex_1))));
	}
}

int power::compare_same_type(const basic & other) const
{
	GINAC_ASSERT(is_exactly_a<power>(other));
	const power &o = static_cast<const power &>(other);

	int cmpval = basis.compare(o.basis);
	if (cmpval)
		return cmpval;
	else
		return exponent.compare(o.exponent);
}

unsigned power::return_type(void) const
{
	return basis.return_type();
}
   
unsigned power::return_type_tinfo(void) const
{
	return basis.return_type_tinfo();
}

ex power::expand(unsigned options) const
{
	if (options == 0 && (flags & status_flags::expanded))
		return *this;
	
	const ex expanded_basis = basis.expand(options);
	const ex expanded_exponent = exponent.expand(options);
	
	// x^(a+b) -> x^a * x^b
	if (is_exactly_a<add>(expanded_exponent)) {
		const add &a = ex_to<add>(expanded_exponent);
		exvector distrseq;
		distrseq.reserve(a.seq.size() + 1);
		epvector::const_iterator last = a.seq.end();
		epvector::const_iterator cit = a.seq.begin();
		while (cit!=last) {
			distrseq.push_back(power(expanded_basis, a.recombine_pair_to_ex(*cit)));
			++cit;
		}
		
		// Make sure that e.g. (x+y)^(2+a) expands the (x+y)^2 factor
		if (ex_to<numeric>(a.overall_coeff).is_integer()) {
			const numeric &num_exponent = ex_to<numeric>(a.overall_coeff);
			int int_exponent = num_exponent.to_int();
			if (int_exponent > 0 && is_exactly_a<add>(expanded_basis))
				distrseq.push_back(expand_add(ex_to<add>(expanded_basis), int_exponent));
			else
				distrseq.push_back(power(expanded_basis, a.overall_coeff));
		} else
			distrseq.push_back(power(expanded_basis, a.overall_coeff));
		
		// Make sure that e.g. (x+y)^(1+a) -> x*(x+y)^a + y*(x+y)^a
		ex r = (new mul(distrseq))->setflag(status_flags::dynallocated);
		return r.expand();
	}
	
	if (!is_exactly_a<numeric>(expanded_exponent) ||
		!ex_to<numeric>(expanded_exponent).is_integer()) {
		if (are_ex_trivially_equal(basis,expanded_basis) && are_ex_trivially_equal(exponent,expanded_exponent)) {
			return this->hold();
		} else {
			return (new power(expanded_basis,expanded_exponent))->setflag(status_flags::dynallocated | (options == 0 ? status_flags::expanded : 0));
		}
	}
	
	// integer numeric exponent
	const numeric & num_exponent = ex_to<numeric>(expanded_exponent);
	int int_exponent = num_exponent.to_int();
	
	// (x+y)^n, n>0
	if (int_exponent > 0 && is_exactly_a<add>(expanded_basis))
		return expand_add(ex_to<add>(expanded_basis), int_exponent);
	
	// (x*y)^n -> x^n * y^n
	if (is_exactly_a<mul>(expanded_basis))
		return expand_mul(ex_to<mul>(expanded_basis), num_exponent);
	
	// cannot expand further
	if (are_ex_trivially_equal(basis,expanded_basis) && are_ex_trivially_equal(exponent,expanded_exponent))
		return this->hold();
	else
		return (new power(expanded_basis,expanded_exponent))->setflag(status_flags::dynallocated | (options == 0 ? status_flags::expanded : 0));
}

//////////
// new virtual functions which can be overridden by derived classes
//////////

// none

//////////
// non-virtual functions in this class
//////////

/** expand a^n where a is an add and n is a positive integer.
 *  @see power::expand */
ex power::expand_add(const add & a, int n) const
{
	if (n==2)
		return expand_add_2(a);

	const int m = a.nops();
	exvector result;
	// The number of terms will be the number of combinatorial compositions,
	// i.e. the number of unordered arrangement of m nonnegative integers
	// which sum up to n.  It is frequently written as C_n(m) and directly
	// related with binomial coefficients:
	result.reserve(binomial(numeric(n+m-1), numeric(m-1)).to_int());
	intvector k(m-1);
	intvector k_cum(m-1); // k_cum[l]:=sum(i=0,l,k[l]);
	intvector upper_limit(m-1);
	int l;

	for (int l=0; l<m-1; ++l) {
		k[l] = 0;
		k_cum[l] = 0;
		upper_limit[l] = n;
	}

	while (true) {
		exvector term;
		term.reserve(m+1);
		for (l=0; l<m-1; ++l) {
			const ex & b = a.op(l);
			GINAC_ASSERT(!is_exactly_a<add>(b));
			GINAC_ASSERT(!is_exactly_a<power>(b) ||
			             !is_exactly_a<numeric>(ex_to<power>(b).exponent) ||
			             !ex_to<numeric>(ex_to<power>(b).exponent).is_pos_integer() ||
			             !is_exactly_a<add>(ex_to<power>(b).basis) ||
			             !is_exactly_a<mul>(ex_to<power>(b).basis) ||
			             !is_exactly_a<power>(ex_to<power>(b).basis));
			if (is_exactly_a<mul>(b))
				term.push_back(expand_mul(ex_to<mul>(b),numeric(k[l])));
			else
				term.push_back(power(b,k[l]));
		}

		const ex & b = a.op(l);
		GINAC_ASSERT(!is_exactly_a<add>(b));
		GINAC_ASSERT(!is_exactly_a<power>(b) ||
		             !is_exactly_a<numeric>(ex_to<power>(b).exponent) ||
		             !ex_to<numeric>(ex_to<power>(b).exponent).is_pos_integer() ||
		             !is_exactly_a<add>(ex_to<power>(b).basis) ||
		             !is_exactly_a<mul>(ex_to<power>(b).basis) ||
		             !is_exactly_a<power>(ex_to<power>(b).basis));
		if (is_exactly_a<mul>(b))
			term.push_back(expand_mul(ex_to<mul>(b),numeric(n-k_cum[m-2])));
		else
			term.push_back(power(b,n-k_cum[m-2]));

		numeric f = binomial(numeric(n),numeric(k[0]));
		for (l=1; l<m-1; ++l)
			f *= binomial(numeric(n-k_cum[l-1]),numeric(k[l]));

		term.push_back(f);

		result.push_back((new mul(term))->setflag(status_flags::dynallocated));

		// increment k[]
		l = m-2;
		while ((l>=0) && ((++k[l])>upper_limit[l])) {
			k[l] = 0;
			--l;
		}
		if (l<0) break;

		// recalc k_cum[] and upper_limit[]
		k_cum[l] = (l==0 ? k[0] : k_cum[l-1]+k[l]);

		for (int i=l+1; i<m-1; ++i)
			k_cum[i] = k_cum[i-1]+k[i];

		for (int i=l+1; i<m-1; ++i)
			upper_limit[i] = n-k_cum[i-1];
	}

	return (new add(result))->setflag(status_flags::dynallocated |
	                                  status_flags::expanded);
}


/** Special case of power::expand_add. Expands a^2 where a is an add.
 *  @see power::expand_add */
ex power::expand_add_2(const add & a) const
{
	epvector sum;
	unsigned a_nops = a.nops();
	sum.reserve((a_nops*(a_nops+1))/2);
	epvector::const_iterator last = a.seq.end();

	// power(+(x,...,z;c),2)=power(+(x,...,z;0),2)+2*c*+(x,...,z;0)+c*c
	// first part: ignore overall_coeff and expand other terms
	for (epvector::const_iterator cit0=a.seq.begin(); cit0!=last; ++cit0) {
		const ex & r = cit0->rest;
		const ex & c = cit0->coeff;
		
		GINAC_ASSERT(!is_exactly_a<add>(r));
		GINAC_ASSERT(!is_exactly_a<power>(r) ||
		             !is_exactly_a<numeric>(ex_to<power>(r).exponent) ||
		             !ex_to<numeric>(ex_to<power>(r).exponent).is_pos_integer() ||
		             !is_exactly_a<add>(ex_to<power>(r).basis) ||
		             !is_exactly_a<mul>(ex_to<power>(r).basis) ||
		             !is_exactly_a<power>(ex_to<power>(r).basis));
		
		if (c.is_equal(_ex1)) {
			if (is_exactly_a<mul>(r)) {
				sum.push_back(expair(expand_mul(ex_to<mul>(r),_num2),
				                     _ex1));
			} else {
				sum.push_back(expair((new power(r,_ex2))->setflag(status_flags::dynallocated),
				                     _ex1));
			}
		} else {
			if (is_exactly_a<mul>(r)) {
				sum.push_back(expair(expand_mul(ex_to<mul>(r),_num2),
				                     ex_to<numeric>(c).power_dyn(_num2)));
			} else {
				sum.push_back(expair((new power(r,_ex2))->setflag(status_flags::dynallocated),
				                     ex_to<numeric>(c).power_dyn(_num2)));
			}
		}

		for (epvector::const_iterator cit1=cit0+1; cit1!=last; ++cit1) {
			const ex & r1 = cit1->rest;
			const ex & c1 = cit1->coeff;
			sum.push_back(a.combine_ex_with_coeff_to_pair((new mul(r,r1))->setflag(status_flags::dynallocated),
			                                              _num2.mul(ex_to<numeric>(c)).mul_dyn(ex_to<numeric>(c1))));
		}
	}
	
	GINAC_ASSERT(sum.size()==(a.seq.size()*(a.seq.size()+1))/2);
	
	// second part: add terms coming from overall_factor (if != 0)
	if (!a.overall_coeff.is_zero()) {
		epvector::const_iterator i = a.seq.begin(), end = a.seq.end();
		while (i != end) {
			sum.push_back(a.combine_pair_with_coeff_to_pair(*i, ex_to<numeric>(a.overall_coeff).mul_dyn(_num2)));
			++i;
		}
		sum.push_back(expair(ex_to<numeric>(a.overall_coeff).power_dyn(_num2),_ex1));
	}
	
	GINAC_ASSERT(sum.size()==(a_nops*(a_nops+1))/2);
	
	return (new add(sum))->setflag(status_flags::dynallocated | status_flags::expanded);
}

/** Expand factors of m in m^n where m is a mul and n is and integer.
 *  @see power::expand */
ex power::expand_mul(const mul & m, const numeric & n) const
{
	GINAC_ASSERT(n.is_integer());

	if (n.is_zero())
		return _ex1;

	epvector distrseq;
	distrseq.reserve(m.seq.size());
	epvector::const_iterator last = m.seq.end();
	epvector::const_iterator cit = m.seq.begin();
	while (cit!=last) {
		if (is_exactly_a<numeric>((*cit).rest)) {
			distrseq.push_back(m.combine_pair_with_coeff_to_pair(*cit,n));
		} else {
			// it is safe not to call mul::combine_pair_with_coeff_to_pair()
			// since n is an integer
			distrseq.push_back(expair((*cit).rest, ex_to<numeric>((*cit).coeff).mul(n)));
		}
		++cit;
	}
	return (new mul(distrseq,ex_to<numeric>(m.overall_coeff).power_dyn(n)))->setflag(status_flags::dynallocated);
}

} // namespace GiNaC
