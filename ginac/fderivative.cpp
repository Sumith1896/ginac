/** @file fderivative.cpp
 *
 *  Implementation of abstract derivatives of functions. */

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

#include <iostream>

#include "fderivative.h"
#include "operators.h"
#include "print.h"
#include "archive.h"
#include "utils.h"

namespace GiNaC {

GINAC_IMPLEMENT_REGISTERED_CLASS(fderivative, function)

//////////
// default ctor, dtor, copy ctor, assignment operator and helpers
//////////

fderivative::fderivative()
{
	tinfo_key = TINFO_fderivative;
}

void fderivative::copy(const fderivative & other)
{
	inherited::copy(other);
	parameter_set = other.parameter_set;
}

DEFAULT_DESTROY(fderivative)

//////////
// other constructors
//////////

fderivative::fderivative(unsigned ser, unsigned param, const exvector & args) : function(ser, args)
{
	parameter_set.insert(param);
	tinfo_key = TINFO_fderivative;
}

fderivative::fderivative(unsigned ser, const paramset & params, const exvector & args) : function(ser, args), parameter_set(params)
{
	tinfo_key = TINFO_fderivative;
}

fderivative::fderivative(unsigned ser, const paramset & params, exvector * vp) : function(ser, vp), parameter_set(params)
{
	tinfo_key = TINFO_fderivative;
}

//////////
// archiving
//////////

fderivative::fderivative(const archive_node &n, const lst &sym_lst) : inherited(n, sym_lst)
{
	unsigned i = 0;
	while (true) {
		unsigned u;
		if (n.find_unsigned("param", u, i))
			parameter_set.insert(u);
		else
			break;
		++i;
	}
}

void fderivative::archive(archive_node &n) const
{
	inherited::archive(n);
	paramset::const_iterator i = parameter_set.begin(), end = parameter_set.end();
	while (i != end) {
		n.add_unsigned("param", *i);
		++i;
	}
}

DEFAULT_UNARCHIVE(fderivative)

//////////
// functions overriding virtual functions from base classes
//////////

void fderivative::print(const print_context & c, unsigned level) const
{
	if (is_a<print_tree>(c)) {

		c.s << std::string(level, ' ') << class_name() << " "
		    << registered_functions()[serial].name
		    << std::hex << ", hash=0x" << hashvalue << ", flags=0x" << flags << std::dec
		    << ", nops=" << nops()
		    << ", params=";
		paramset::const_iterator i = parameter_set.begin(), end = parameter_set.end();
		--end;
		while (i != end)
			c.s << *i++ << ",";
		c.s << *i << std::endl;
		unsigned delta_indent = static_cast<const print_tree &>(c).delta_indent;
		for (unsigned i=0; i<seq.size(); ++i)
			seq[i].print(c, level + delta_indent);
		c.s << std::string(level + delta_indent, ' ') << "=====" << std::endl;

	} else {

		c.s << "D[";
		paramset::const_iterator i = parameter_set.begin(), end = parameter_set.end();
		--end;
		while (i != end)
			c.s << *i++ << ",";
		c.s << *i << "](" << registered_functions()[serial].name << ")";
		printseq(c, '(', ',', ')', exprseq::precedence(), function::precedence());
	}
}

ex fderivative::eval(int level) const
{
	if (level > 1) {
		// first evaluate children, then we will end up here again
		return fderivative(serial, parameter_set, evalchildren(level));
	}

	// No parameters specified? Then return the function itself
	if (parameter_set.empty())
		return function(serial, seq);

	// If the function in question actually has a derivative, return it
	if (registered_functions()[serial].has_derivative() && parameter_set.size() == 1)
		return pderivative(*(parameter_set.begin()));

	return this->hold();
}

/** Numeric evaluation falls back to evaluation of arguments.
 *  @see basic::evalf */
ex fderivative::evalf(int level) const
{
	return basic::evalf(level);
}

/** The series expansion of derivatives falls back to Taylor expansion.
 *  @see basic::series */
ex fderivative::series(const relational & r, int order, unsigned options) const
{
	return basic::series(r, order, options);
}

ex fderivative::thisexprseq(const exvector & v) const
{
	return fderivative(serial, parameter_set, v);
}

ex fderivative::thisexprseq(exvector * vp) const
{
	return fderivative(serial, parameter_set, vp);
}

/** Implementation of ex::diff() for derivatives. It applies the chain rule.
 *  @see ex::diff */
ex fderivative::derivative(const symbol & s) const
{
	ex result;
	for (unsigned i=0; i!=seq.size(); i++) {
		ex arg_diff = seq[i].diff(s);
		if (!arg_diff.is_zero()) {
			paramset ps = parameter_set;
			ps.insert(i);
			result += arg_diff * fderivative(serial, ps, seq);
		}
	}
	return result;
}

int fderivative::compare_same_type(const basic & other) const
{
	GINAC_ASSERT(is_a<fderivative>(other));
	const fderivative & o = static_cast<const fderivative &>(other);

	if (parameter_set != o.parameter_set)
		return parameter_set < o.parameter_set ? -1 : 1;
	else
		return inherited::compare_same_type(o);
}

bool fderivative::is_equal_same_type(const basic & other) const
{
	GINAC_ASSERT(is_a<fderivative>(other));
	const fderivative & o = static_cast<const fderivative &>(other);

	if (parameter_set != o.parameter_set)
		return false;
	else
		return inherited::is_equal_same_type(o);
}

bool fderivative::match_same_type(const basic & other) const
{
	GINAC_ASSERT(is_a<fderivative>(other));
	const fderivative & o = static_cast<const fderivative &>(other);

	return parameter_set == o.parameter_set;
}

} // namespace GiNaC
