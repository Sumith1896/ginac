/** @file basic.cpp
 *
 *  Implementation of GiNaC's ABC. */

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

#include <iostream>
#include <stdexcept>
#ifdef DO_GINAC_ASSERT
#  include <typeinfo>
#endif

#include "basic.h"
#include "ex.h"
#include "numeric.h"
#include "power.h"
#include "symbol.h"
#include "lst.h"
#include "ncmul.h"
#include "relational.h"
#include "operators.h"
#include "wildcard.h"
#include "print.h"
#include "archive.h"
#include "utils.h"

namespace GiNaC {

GINAC_IMPLEMENT_REGISTERED_CLASS_NO_CTORS(basic, void)

//////////
// default ctor, dtor, copy ctor, assignment operator and helpers
//////////

// public

basic::basic(const basic & other) : tinfo_key(TINFO_basic), flags(0), refcount(0)
{
	copy(other);
}

const basic & basic::operator=(const basic & other)
{
	if (this != &other) {
		destroy(true);
		copy(other);
	}
	return *this;
}

// protected

// none (all conditionally inlined)

//////////
// other ctors
//////////

// none (all conditionally inlined)

//////////
// archiving
//////////

/** Construct object from archive_node. */
basic::basic(const archive_node &n, const lst &sym_lst) : flags(0), refcount(0)
{
	// Reconstruct tinfo_key from class name
	std::string class_name;
	if (n.find_string("class", class_name))
		tinfo_key = find_tinfo_key(class_name);
	else
		throw (std::runtime_error("archive node contains no class name"));
}

/** Unarchive the object. */
DEFAULT_UNARCHIVE(basic)

/** Archive the object. */
void basic::archive(archive_node &n) const
{
	n.add_string("class", class_name());
}

//////////
// new virtual functions which can be overridden by derived classes
//////////

// public

/** Output to stream.
 *  @param c print context object that describes the output formatting
 *  @param level value that is used to identify the precedence or indentation
 *               level for placing parentheses and formatting */
void basic::print(const print_context & c, unsigned level) const
{
	if (is_a<print_tree>(c)) {

		c.s << std::string(level, ' ') << class_name()
		    << std::hex << ", hash=0x" << hashvalue << ", flags=0x" << flags << std::dec
		    << ", nops=" << nops()
		    << std::endl;
		for (unsigned i=0; i<nops(); ++i)
			op(i).print(c, level + static_cast<const print_tree &>(c).delta_indent);

	} else
		c.s << "[" << class_name() << " object]";
}

/** Little wrapper around print to be called within a debugger.
 *  This is needed because you cannot call foo.print(cout) from within the
 *  debugger because it might not know what cout is.  This method can be
 *  invoked with no argument and it will simply print to stdout.
 *
 *  @see basic::print */
void basic::dbgprint(void) const
{
	this->print(std::cerr);
	std::cerr << std::endl;
}

/** Little wrapper around printtree to be called within a debugger.
 *
 *  @see basic::dbgprint
 *  @see basic::printtree */
void basic::dbgprinttree(void) const
{
	this->print(print_tree(std::cerr));
}

/** Return relative operator precedence (for parenthizing output). */
unsigned basic::precedence(void) const
{
	return 70;
}

/** Create a new copy of this on the heap.  One can think of this as simulating
 *  a virtual copy constructor which is needed for instance by the refcounted
 *  construction of an ex from a basic. */
basic * basic::duplicate() const
{
	return new basic(*this);
}

/** Information about the object.
 *
 *  @see class info_flags */
bool basic::info(unsigned inf) const
{
	// all possible properties are false for basic objects
	return false;
}

/** Number of operands/members. */
unsigned basic::nops() const
{
	// iterating from 0 to nops() on atomic objects should be an empty loop,
	// and accessing their elements is a range error.  Container objects should
	// override this.
	return 0;
}

/** Return operand/member at position i. */
ex basic::op(int i) const
{
	throw(std::runtime_error(std::string("op() not implemented by ") + class_name()));
}

/** Return modifyable operand/member at position i. */
ex & basic::let_op(int i)
{
	ensure_if_modifiable();
	throw(std::runtime_error(std::string("let_op() not implemented by ") + class_name()));
}

ex basic::operator[](const ex & index) const
{
	if (is_exactly_a<numeric>(index))
		return op(ex_to<numeric>(index).to_int());

	throw(std::invalid_argument("non-numeric indices not supported by this type"));
}

ex basic::operator[](int i) const
{
	return op(i);
}

ex & basic::operator[](const ex & index)
{
	if (is_exactly_a<numeric>(index))
		return let_op(ex_to<numeric>(index).to_int());

	throw(std::invalid_argument("non-numeric indices not supported by this type"));
}

ex & basic::operator[](int i)
{
	return let_op(i);
}

/** Test for occurrence of a pattern.  An object 'has' a pattern if it matches
 *  the pattern itself or one of the children 'has' it.  As a consequence
 *  (according to the definition of children) given e=x+y+z, e.has(x) is true
 *  but e.has(x+y) is false. */
bool basic::has(const ex & pattern) const
{
	lst repl_lst;
	if (match(pattern, repl_lst))
		return true;
	for (unsigned i=0; i<nops(); i++)
		if (op(i).has(pattern))
			return true;
	
	return false;
}

/** Construct new expression by applying the specified function to all
 *  sub-expressions (one level only, not recursively). */
ex basic::map(map_function & f) const
{
	unsigned num = nops();
	if (num == 0)
		return *this;

	basic *copy = duplicate();
	copy->setflag(status_flags::dynallocated);
	copy->clearflag(status_flags::hash_calculated | status_flags::expanded);
	ex e(*copy);
	for (unsigned i=0; i<num; i++)
		e.let_op(i) = f(e.op(i));
	return e.eval();
}

/** Return degree of highest power in object s. */
int basic::degree(const ex & s) const
{
	return is_equal(ex_to<basic>(s)) ? 1 : 0;
}

/** Return degree of lowest power in object s. */
int basic::ldegree(const ex & s) const
{
	return is_equal(ex_to<basic>(s)) ? 1 : 0;
}

/** Return coefficient of degree n in object s. */
ex basic::coeff(const ex & s, int n) const
{
	if (is_equal(ex_to<basic>(s)))
		return n==1 ? _ex1 : _ex0;
	else
		return n==0 ? *this : _ex0;
}

/** Sort expanded expression in terms of powers of some object(s).
 *  @param s object(s) to sort in
 *  @param distributed recursive or distributed form (only used when s is a list) */
ex basic::collect(const ex & s, bool distributed) const
{
	ex x;
	if (is_a<lst>(s)) {

		// List of objects specified
		if (s.nops() == 0)
			return *this;
		if (s.nops() == 1)
			return collect(s.op(0));

		else if (distributed) {

			// Get lower/upper degree of all symbols in list
			int num = s.nops();
			struct sym_info {
				ex sym;
				int ldeg, deg;
				int cnt;  // current degree, 'counter'
				ex coeff; // coefficient for degree 'cnt'
			};
			sym_info *si = new sym_info[num];
			ex c = *this;
			for (int i=0; i<num; i++) {
				si[i].sym = s.op(i);
				si[i].ldeg = si[i].cnt = this->ldegree(si[i].sym);
				si[i].deg = this->degree(si[i].sym);
				c = si[i].coeff = c.coeff(si[i].sym, si[i].cnt);
			}

			while (true) {

				// Calculate coeff*x1^c1*...*xn^cn
				ex y = _ex1;
				for (int i=0; i<num; i++) {
					int cnt = si[i].cnt;
					y *= power(si[i].sym, cnt);
				}
				x += y * si[num - 1].coeff;

				// Increment counters
				int n = num - 1;
				while (true) {
					++si[n].cnt;
					if (si[n].cnt <= si[n].deg) {
						// Update coefficients
						ex c;
						if (n == 0)
							c = *this;
						else
							c = si[n - 1].coeff;
						for (int i=n; i<num; i++)
							c = si[i].coeff = c.coeff(si[i].sym, si[i].cnt);
						break;
					}
					if (n == 0)
						goto done;
					si[n].cnt = si[n].ldeg;
					n--;
				}
			}

done:		delete[] si;

		} else {

			// Recursive form
			x = *this;
			for (int n=s.nops()-1; n>=0; n--)
				x = x.collect(s[n]);
		}

	} else {

		// Only one object specified
		for (int n=this->ldegree(s); n<=this->degree(s); ++n)
			x += this->coeff(s,n)*power(s,n);
	}
	
	// correct for lost fractional arguments and return
	return x + (*this - x).expand();
}

/** Perform automatic non-interruptive term rewriting rules. */
ex basic::eval(int level) const
{
	// There is nothing to do for basic objects:
	return this->hold();
}

/** Function object to be applied by basic::evalf(). */
struct evalf_map_function : public map_function {
	int level;
	evalf_map_function(int l) : level(l) {}
	ex operator()(const ex & e) { return evalf(e, level); }
};

/** Evaluate object numerically. */
ex basic::evalf(int level) const
{
	if (nops() == 0)
		return *this;
	else {
		if (level == 1)
			return *this;
		else if (level == -max_recursion_level)
			throw(std::runtime_error("max recursion level reached"));
		else {
			evalf_map_function map_evalf(level - 1);
			return map(map_evalf);
		}
	}
}

/** Function object to be applied by basic::evalm(). */
struct evalm_map_function : public map_function {
	ex operator()(const ex & e) { return evalm(e); }
} map_evalm;

/** Evaluate sums, products and integer powers of matrices. */
ex basic::evalm(void) const
{
	if (nops() == 0)
		return *this;
	else
		return map(map_evalm);
}

/** Perform automatic symbolic evaluations on indexed expression that
 *  contains this object as the base expression. */
ex basic::eval_indexed(const basic & i) const
 // this function can't take a "const ex & i" because that would result
 // in an infinite eval() loop
{
	// There is nothing to do for basic objects
	return i.hold();
}

/** Add two indexed expressions. They are guaranteed to be of class indexed
 *  (or a subclass) and their indices are compatible. This function is used
 *  internally by simplify_indexed().
 *
 *  @param self First indexed expression; it's base object is *this
 *  @param other Second indexed expression
 *  @return sum of self and other 
 *  @see ex::simplify_indexed() */
ex basic::add_indexed(const ex & self, const ex & other) const
{
	return self + other;
}

/** Multiply an indexed expression with a scalar. This function is used
 *  internally by simplify_indexed().
 *
 *  @param self Indexed expression; it's base object is *this
 *  @param other Numeric value
 *  @return product of self and other
 *  @see ex::simplify_indexed() */
ex basic::scalar_mul_indexed(const ex & self, const numeric & other) const
{
	return self * other;
}

/** Try to contract two indexed expressions that appear in the same product. 
 *  If a contraction exists, the function overwrites one or both of the
 *  expressions and returns true. Otherwise it returns false. It is
 *  guaranteed that both expressions are of class indexed (or a subclass)
 *  and that at least one dummy index has been found. This functions is
 *  used internally by simplify_indexed().
 *
 *  @param self Pointer to first indexed expression; it's base object is *this
 *  @param other Pointer to second indexed expression
 *  @param v The complete vector of factors
 *  @return true if the contraction was successful, false otherwise
 *  @see ex::simplify_indexed() */
bool basic::contract_with(exvector::iterator self, exvector::iterator other, exvector & v) const
{
	// Do nothing
	return false;
}

/** Check whether the expression matches a given pattern. For every wildcard
 *  object in the pattern, an expression of the form "wildcard == matching_expression"
 *  is added to repl_lst. */
bool basic::match(const ex & pattern, lst & repl_lst) const
{
/*
	Sweet sweet shapes, sweet sweet shapes,
	That's the key thing, right right.
	Feed feed face, feed feed shapes,
	But who is the king tonight?
	Who is the king tonight?
	Pattern is the thing, the key thing-a-ling,
	But who is the king of Pattern?
	But who is the king, the king thing-a-ling,
	Who is the king of Pattern?
	Bog is the king, the king thing-a-ling,
	Bog is the king of Pattern.
	Ba bu-bu-bu-bu bu-bu-bu-bu-bu-bu bu-bu
	Bog is the king of Pattern.
*/

	if (is_exactly_a<wildcard>(pattern)) {

		// Wildcard matches anything, but check whether we already have found
		// a match for that wildcard first (if so, it the earlier match must
		// be the same expression)
		for (unsigned i=0; i<repl_lst.nops(); i++) {
			if (repl_lst.op(i).op(0).is_equal(pattern))
				return is_equal(ex_to<basic>(repl_lst.op(i).op(1)));
		}
		repl_lst.append(pattern == *this);
		return true;

	} else {

		// Expression must be of the same type as the pattern
		if (tinfo() != ex_to<basic>(pattern).tinfo())
			return false;

		// Number of subexpressions must match
		if (nops() != pattern.nops())
			return false;

		// No subexpressions? Then just compare the objects (there can't be
		// wildcards in the pattern)
		if (nops() == 0)
			return is_equal_same_type(ex_to<basic>(pattern));

		// Check whether attributes that are not subexpressions match
		if (!match_same_type(ex_to<basic>(pattern)))
			return false;

		// Otherwise the subexpressions must match one-to-one
		for (unsigned i=0; i<nops(); i++)
			if (!op(i).match(pattern.op(i), repl_lst))
				return false;

		// Looks similar enough, match found
		return true;
	}
}

/** Substitute a set of objects by arbitrary expressions. The ex returned
 *  will already be evaluated. */
ex basic::subs(const lst & ls, const lst & lr, unsigned options) const
{
	GINAC_ASSERT(ls.nops() == lr.nops());

	if (options & subs_options::subs_no_pattern) {
		for (unsigned i=0; i<ls.nops(); i++) {
			if (is_equal(ex_to<basic>(ls.op(i))))
				return lr.op(i);
		}
	} else {
		for (unsigned i=0; i<ls.nops(); i++) {
			lst repl_lst;
			if (match(ex_to<basic>(ls.op(i)), repl_lst))
				return lr.op(i).subs(repl_lst, options | subs_options::subs_no_pattern); // avoid infinite recursion when re-substituting the wildcards
		}
	}

	return *this;
}

/** Default interface of nth derivative ex::diff(s, n).  It should be called
 *  instead of ::derivative(s) for first derivatives and for nth derivatives it
 *  just recurses down.
 *
 *  @param s symbol to differentiate in
 *  @param nth order of differentiation
 *  @see ex::diff */
ex basic::diff(const symbol & s, unsigned nth) const
{
	// trivial: zeroth derivative
	if (nth==0)
		return ex(*this);
	
	// evaluate unevaluated *this before differentiating
	if (!(flags & status_flags::evaluated))
		return ex(*this).diff(s, nth);
	
	ex ndiff = this->derivative(s);
	while (!ndiff.is_zero() &&    // stop differentiating zeros
	       nth>1) {
		ndiff = ndiff.diff(s);
		--nth;
	}
	return ndiff;
}

/** Return a vector containing the free indices of an expression. */
exvector basic::get_free_indices(void) const
{
	return exvector(); // return an empty exvector
}

ex basic::eval_ncmul(const exvector & v) const
{
	return simplified_ncmul(v);
}

// protected

/** Function object to be applied by basic::derivative(). */
struct derivative_map_function : public map_function {
	const symbol &s;
	derivative_map_function(const symbol &sym) : s(sym) {}
	ex operator()(const ex & e) { return diff(e, s); }
};

/** Default implementation of ex::diff(). It maps the operation on the
 *  operands (or returns 0 when the object has no operands).
 *
 *  @see ex::diff */
ex basic::derivative(const symbol & s) const
{
	if (nops() == 0)
		return _ex0;
	else {
		derivative_map_function map_derivative(s);
		return map(map_derivative);
	}
}

/** Returns order relation between two objects of same type.  This needs to be
 *  implemented by each class. It may never return anything else than 0,
 *  signalling equality, or +1 and -1 signalling inequality and determining
 *  the canonical ordering.  (Perl hackers will wonder why C++ doesn't feature
 *  the spaceship operator <=> for denoting just this.) */
int basic::compare_same_type(const basic & other) const
{
	return compare_pointers(this, &other);
}

/** Returns true if two objects of same type are equal.  Normally needs
 *  not be reimplemented as long as it wasn't overwritten by some parent
 *  class, since it just calls compare_same_type().  The reason why this
 *  function exists is that sometimes it is easier to determine equality
 *  than an order relation and then it can be overridden. */
bool basic::is_equal_same_type(const basic & other) const
{
	return compare_same_type(other)==0;
}

/** Returns true if the attributes of two objects are similar enough for
 *  a match. This function must not match subexpressions (this is already
 *  done by basic::match()). Only attributes not accessible by op() should
 *  be compared. This is also the reason why this function doesn't take the
 *  wildcard replacement list from match() as an argument: only subexpressions
 *  are subject to wildcard matches. Also, this function only needs to be
 *  implemented for container classes because is_equal_same_type() is
 *  automatically used instead of match_same_type() if nops() == 0.
 *
 *  @see basic::match */
bool basic::match_same_type(const basic & other) const
{
	// The default is to only consider subexpressions, but not any other
	// attributes
	return true;
}

unsigned basic::return_type(void) const
{
	return return_types::commutative;
}

unsigned basic::return_type_tinfo(void) const
{
	return tinfo();
}

/** Compute the hash value of an object and if it makes sense to store it in
 *  the objects status_flags, do so.  The method inherited from class basic
 *  computes a hash value based on the type and hash values of possible
 *  members.  For this reason it is well suited for container classes but
 *  atomic classes should override this implementation because otherwise they
 *  would all end up with the same hashvalue. */
unsigned basic::calchash(void) const
{
	unsigned v = golden_ratio_hash(tinfo());
	for (unsigned i=0; i<nops(); i++) {
		v = rotate_left(v);
		v ^= (const_cast<basic *>(this))->op(i).gethash();
	}

	// store calculated hash value only if object is already evaluated
	if (flags & status_flags::evaluated) {
		setflag(status_flags::hash_calculated);
		hashvalue = v;
	}

	return v;
}

/** Function object to be applied by basic::expand(). */
struct expand_map_function : public map_function {
	unsigned options;
	expand_map_function(unsigned o) : options(o) {}
	ex operator()(const ex & e) { return expand(e, options); }
};

/** Expand expression, i.e. multiply it out and return the result as a new
 *  expression. */
ex basic::expand(unsigned options) const
{
	if (nops() == 0)
		return (options == 0) ? setflag(status_flags::expanded) : *this;
	else {
		expand_map_function map_expand(options);
		return ex_to<basic>(map(map_expand)).setflag(options == 0 ? status_flags::expanded : 0);
	}
}


//////////
// non-virtual functions in this class
//////////

// public

/** Substitute objects in an expression (syntactic substitution) and return
 *  the result as a new expression.  There are two valid types of
 *  replacement arguments: 1) a relational like object==ex and 2) a list of
 *  relationals lst(object1==ex1,object2==ex2,...), which is converted to
 *  subs(lst(object1,object2,...),lst(ex1,ex2,...)). */
ex basic::subs(const ex & e, unsigned options) const
{
	if (e.info(info_flags::relation_equal)) {
		return subs(lst(e), options);
	}
	if (!e.info(info_flags::list)) {
		throw(std::invalid_argument("basic::subs(ex): argument must be a list"));
	}
	lst ls;
	lst lr;
	for (unsigned i=0; i<e.nops(); i++) {
		ex r = e.op(i);
		if (!r.info(info_flags::relation_equal)) {
			throw(std::invalid_argument("basic::subs(ex): argument must be a list of equations"));
		}
		ls.append(r.op(0));
		lr.append(r.op(1));
	}
	return subs(ls, lr, options);
}

/** Compare objects syntactically to establish canonical ordering.
 *  All compare functions return: -1 for *this less than other, 0 equal,
 *  1 greater. */
int basic::compare(const basic & other) const
{
	const unsigned hash_this = gethash();
	const unsigned hash_other = other.gethash();
	if (hash_this<hash_other) return -1;
	if (hash_this>hash_other) return 1;

	const unsigned typeid_this = tinfo();
	const unsigned typeid_other = other.tinfo();
	if (typeid_this==typeid_other) {
		GINAC_ASSERT(typeid(*this)==typeid(other));
// 		int cmpval = compare_same_type(other);
// 		if (cmpval!=0) {
// 			std::cout << "hash collision, same type: " 
// 			          << *this << " and " << other << std::endl;
// 			this->print(print_tree(std::cout));
// 			std::cout << " and ";
// 			other.print(print_tree(std::cout));
// 			std::cout << std::endl;
// 		}
// 		return cmpval;
		return compare_same_type(other);
	} else {
// 		std::cout << "hash collision, different types: " 
// 		          << *this << " and " << other << std::endl;
// 		this->print(print_tree(std::cout));
// 		std::cout << " and ";
// 		other.print(print_tree(std::cout));
// 		std::cout << std::endl;
		return (typeid_this<typeid_other ? -1 : 1);
	}
}

/** Test for syntactic equality.
 *  This is only a quick test, meaning objects should be in the same domain.
 *  You might have to .expand(), .normal() objects first, depending on the
 *  domain of your computation, to get a more reliable answer.
 *
 *  @see is_equal_same_type */
bool basic::is_equal(const basic & other) const
{
	if (this->gethash()!=other.gethash())
		return false;
	if (this->tinfo()!=other.tinfo())
		return false;
	
	GINAC_ASSERT(typeid(*this)==typeid(other));
	
	return is_equal_same_type(other);
}

// protected

/** Stop further evaluation.
 *
 *  @see basic::eval */
const basic & basic::hold(void) const
{
	return setflag(status_flags::evaluated);
}

/** Ensure the object may be modified without hurting others, throws if this
 *  is not the case. */
void basic::ensure_if_modifiable(void) const
{
	if (this->refcount>1)
		throw(std::runtime_error("cannot modify multiply referenced object"));
	clearflag(status_flags::hash_calculated);
}

//////////
// global variables
//////////

int max_recursion_level = 1024;

} // namespace GiNaC
