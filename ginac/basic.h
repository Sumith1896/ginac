/** @file basic.h
 *
 *  Interface to GiNaC's ABC. */

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

#ifndef __GINAC_BASIC_H__
#define __GINAC_BASIC_H__

#include <cstddef> // for size_t
#include <vector>
// CINT needs <algorithm> to work properly with <vector>
#include <algorithm>

#include "flags.h"
#include "tinfos.h"
#include "assertion.h"
#include "registrar.h"

namespace GiNaC {

class ex;
class ex_is_less;
class symbol;
class lst;
class numeric;
class relational;
class archive_node;
class print_context;

typedef std::vector<ex> exvector;


/** Function object for map(). */
struct map_function {
	typedef const ex & argument_type;
	typedef ex result_type;
	virtual ex operator()(const ex & e) = 0;
};


/** This class is the ABC (abstract base class) of GiNaC's class hierarchy.
 *  It is responsible for the reference counting. */
class basic
{
	GINAC_DECLARE_REGISTERED_CLASS_NO_CTORS(basic, void)
	
	friend class ex;
	
	// default ctor, dtor, copy ctor, assignment operator and helpers
public:
	basic() : tinfo_key(TINFO_basic), flags(0), refcount(0) {}
	/** basic dtor, virtual because class ex will delete objects via ptr. */
	virtual ~basic()
	{
		destroy(false);
		GINAC_ASSERT((!(flags & status_flags::dynallocated))||(refcount==0));
	}
	basic(const basic & other);
	const basic & operator=(const basic & other);
protected:
	/** For use by copy ctor and assignment operator. */
	void copy(const basic & other)
	{
		flags = other.flags & ~status_flags::dynallocated;
		hashvalue = other.hashvalue;
		tinfo_key = other.tinfo_key;
	}
	/** For use by dtor and assignment operator. */
	virtual void destroy(bool call_parent) { }
	
	// other ctors
	/** ctor with specified tinfo_key */
	basic(unsigned ti) : tinfo_key(ti), flags(0), refcount(0) {}
	
	// new virtual functions which can be overridden by derived classes
public: // only const functions please (may break reference counting)
	virtual basic * duplicate() const;
	virtual void print(const print_context & c, unsigned level = 0) const;
	virtual void dbgprint(void) const;
	virtual void dbgprinttree(void) const;
	virtual unsigned precedence(void) const;
	virtual bool info(unsigned inf) const;
	virtual size_t nops() const;
	virtual ex op(size_t i) const;
	virtual ex operator[](const ex & index) const;
	virtual ex operator[](size_t i) const;
	virtual ex & let_op(size_t i);
	virtual ex & operator[](const ex & index);
	virtual ex & operator[](size_t i);
	virtual ex expand(unsigned options = 0) const;
	virtual bool has(const ex & other) const;
	virtual ex map(map_function & f) const;
	virtual int degree(const ex & s) const;
	virtual int ldegree(const ex & s) const;
	virtual ex coeff(const ex & s, int n = 1) const;
	virtual ex collect(const ex & s, bool distributed = false) const;
	virtual ex eval(int level = 0) const;
	virtual ex evalf(int level = 0) const;
	virtual ex evalm(void) const;
	virtual ex series(const relational & r, int order, unsigned options = 0) const;
	virtual bool match(const ex & pattern, lst & repl_lst) const;
	virtual ex subs(const lst & ls, const lst & lr, unsigned options = 0) const;
	virtual ex normal(lst &sym_lst, lst &repl_lst, int level = 0) const;
	virtual ex to_rational(lst &repl_lst) const;
	virtual ex to_polynomial(lst &repl_lst) const;
	virtual numeric integer_content(void) const;
	virtual ex smod(const numeric &xi) const;
	virtual numeric max_coefficient(void) const;
	virtual exvector get_free_indices(void) const;
	virtual ex eval_indexed(const basic & i) const;
	virtual ex add_indexed(const ex & self, const ex & other) const;
	virtual ex scalar_mul_indexed(const ex & self, const numeric & other) const;
	virtual bool contract_with(exvector::iterator self, exvector::iterator other, exvector & v) const;
	virtual unsigned return_type(void) const;
	virtual unsigned return_type_tinfo(void) const;
protected: // functions that should be called from class ex only
	virtual ex derivative(const symbol & s) const;
	virtual int compare_same_type(const basic & other) const;
	virtual bool is_equal_same_type(const basic & other) const;
	virtual bool match_same_type(const basic & other) const;
	virtual unsigned calchash(void) const;
	virtual ex eval_ncmul(const exvector & v) const;
	
	// non-virtual functions in this class
public:
	ex subs(const ex & e, unsigned options = 0) const;
	ex diff(const symbol & s, unsigned nth = 1) const;
	int compare(const basic & other) const;
	bool is_equal(const basic & other) const;
	const basic & hold(void) const;
	unsigned gethash(void) const { if (flags & status_flags::hash_calculated) return hashvalue; else return calchash(); }
	unsigned tinfo(void) const {return tinfo_key;}

	/** Set some status_flags. */
	const basic & setflag(unsigned f) const {flags |= f; return *this;}

	/** Clear some status_flags. */
	const basic & clearflag(unsigned f) const {flags &= ~f; return *this;}

protected:
	void ensure_if_modifiable(void) const;
	
	// member variables
protected:
	unsigned tinfo_key;                 ///< typeinfo
	mutable unsigned flags;             ///< of type status_flags
	mutable unsigned hashvalue;         ///< hash value
private:
	unsigned refcount;                  ///< Number of reference counts
};

// global variables

extern int max_recursion_level;

// convenience type checker template functions

/** Check if obj is a T, including base classes. */
template <class T>
inline bool is_a(const basic &obj)
{
	return dynamic_cast<const T *>(&obj)!=0;
}

/** Check if obj is a T, not including base classes.  This one is just an
 *  inefficient default.  It should in all time-critical cases be overridden
 *  by template specializations that don't create a temporary. */
template <class T>
inline bool is_exactly_a(const class basic &obj)
{
	const T foo; return foo.tinfo()==obj.tinfo();
}

} // namespace GiNaC

#endif // ndef __GINAC_BASIC_H__
