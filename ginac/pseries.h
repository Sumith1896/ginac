/** @file pseries.h
 *
 *  Interface to class for extended truncated power series. */

/*
 *  GiNaC Copyright (C) 1999-2003 Johannes Gutenberg University Mainz, Germany
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

#ifndef __GINAC_SERIES_H__
#define __GINAC_SERIES_H__

#include "basic.h"
#include "expairseq.h"

namespace GiNaC {

/** This class holds a extended truncated power series (positive and negative
 *  integer powers). It consists of expression coefficients (only non-zero
 *  coefficients are stored), an expansion variable and an expansion point.
 *  Other classes must provide members to convert into this type. */
class pseries : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(pseries, basic)

	// other ctors
public:
	pseries(const ex &rel_, const epvector &ops_);

	// functions overriding virtual functions from base classes
public:
	void print(const print_context & c, unsigned level = 0) const;
	unsigned precedence(void) const {return 38;} // for clarity just below add::precedence
	unsigned nops(void) const;
	ex op(int i) const;
	ex &let_op(int i);
	int degree(const ex &s) const;
	int ldegree(const ex &s) const;
	ex coeff(const ex &s, int n = 1) const;
	ex collect(const ex &s, bool distributed = false) const;
	ex eval(int level=0) const;
	ex evalf(int level=0) const;
	ex series(const relational & r, int order, unsigned options = 0) const;
	ex subs(const lst & ls, const lst & lr, unsigned options = 0) const;
	ex normal(lst &sym_lst, lst &repl_lst, int level = 0) const;
	ex expand(unsigned options = 0) const;
protected:
	ex derivative(const symbol & s) const;

	// non-virtual functions in this class
public:
	/** Get the expansion variable. */
	ex get_var(void) const {return var;}

	/** Get the expansion point. */
	ex get_point(void) const {return point;}

	/** Convert the pseries object to an ordinary polynomial.
	 *
	 *  @param no_order flag: discard higher order terms */
	ex convert_to_poly(bool no_order = false) const;

	/** Check whether series is compatible to another series (expansion
	 *  variable and point are the same. */
	bool is_compatible_to(const pseries &other) const {return var.is_equal(other.var) && point.is_equal(other.point);}

	/** Check whether series has the value zero. */
	bool is_zero(void) const {return seq.size() == 0;}

	/** Returns true if there is no order term, i.e. the series terminates and
	 *  false otherwise. */
	bool is_terminating(void) const;

	ex add_series(const pseries &other) const;
	ex mul_const(const numeric &other) const;
	ex mul_series(const pseries &other) const;
	ex power_const(const numeric &p, int deg) const;
	pseries shift_exponents(int deg) const;

protected:
	/** Vector of {coefficient, power} pairs */
	epvector seq;

	/** Series variable (holds a symbol) */
	ex var;

	/** Expansion point */
	ex point;
};


// utility functions

/** Specialization of is_exactly_a<pseries>(obj) for pseries objects. */
template<> inline bool is_exactly_a<pseries>(const basic & obj)
{
	return obj.tinfo()==TINFO_pseries;
}

/** Convert the pseries object embedded in an expression to an ordinary
 *  polynomial in the expansion variable. The result is undefined if the
 *  expression does not contain a pseries object at its top level.
 *
 *  @param e expression
 *  @return polynomial expression
 *  @see is_a<>
 *  @see pseries::convert_to_poly */
inline ex series_to_poly(const ex &e)
{
	return (ex_to<pseries>(e).convert_to_poly(true));
}

inline bool is_terminating(const pseries & s)
{
	return s.is_terminating();
}

} // namespace GiNaC

#endif // ndef __GINAC_SERIES_H__
