/** @file relational.h
 *
 *  Interface to relations between expressions. */

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

#ifndef __GINAC_RELATIONAL_H__
#define __GINAC_RELATIONAL_H__

#include "basic.h"
#include "ex.h"

namespace GiNaC {

/** This class holds a relation consisting of two expressions and a logical
 *  relation between them. */
class relational : public basic
{
	GINAC_DECLARE_REGISTERED_CLASS(relational, basic)

// types
public:
	enum operators {
		equal,
		not_equal,
		less,
		less_or_equal,
		greater,
		greater_or_equal
	};
	
	// other ctors
public:
	relational(const ex & lhs, const ex & rhs, operators oper=equal);
	
	// functions overriding virtual functions from base classes
public:
	void print(const print_context & c, unsigned level = 0) const;
	unsigned precedence(void) const {return 20;}
	bool info(unsigned inf) const;
	unsigned nops() const;
	ex op(int i) const;
	ex map(map_function & f) const;
	ex eval(int level=0) const;

protected:
	ex eval_ncmul(const exvector & v) const;
	bool match_same_type(const basic & other) const;
	unsigned return_type(void) const;
	unsigned return_type_tinfo(void) const;
	unsigned calchash(void) const;

	// new virtual functions which can be overridden by derived classes
public:
	virtual ex lhs(void) const;
	virtual ex rhs(void) const;

	// non-virtual functions in this class
private:
	// For conversions to boolean, as would be used in an if conditional,
	// implicit conversions from bool to int have a large number of
	// undesirable side effects.  The following safe_bool type enables
	// use of relational objects in conditionals without those side effects
	struct safe_bool_helper {
		void nonnull() {};
	};

	typedef void (safe_bool_helper::*safe_bool)();
	
	safe_bool make_safe_bool(bool) const;

public:
	operator safe_bool(void) const;
	safe_bool operator!(void) const;

// member variables
	
protected:
	ex lh;
	ex rh;
	operators o;
};

// utility functions

/** Specialization of is_exactly_a<relational>(obj) for relational objects. */
template<> inline bool is_exactly_a<relational>(const basic & obj)
{
	return obj.tinfo()==TINFO_relational;
}

// inlined functions for efficiency
inline relational::safe_bool relational::operator!() const
{
	return make_safe_bool(!static_cast<bool>(*this));
}

} // namespace GiNaC

#endif // ndef __GINAC_RELATIONAL_H__
