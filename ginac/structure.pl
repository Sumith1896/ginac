#!/usr/bin/perl -w

$input_structure='';
$original_input_structure='';
while (<>) {
	$input_structure .= '// '.$_;
	$original_input_structure .= $_;
}

$original_input_structure =~ tr/ \t\n\r\f/     /;
$original_input_structure =~ tr/ //s;

if ($original_input_structure =~ /^struct (\w+) ?\{ ?(.*)\}\;? ?$/) {
	$STRUCTURE=$1;
	$decl=$2;
} else {
	die "illegal struct, must match 'struct name { type var; /*comment*/ ...};': $original_input_structure";
}

# split off a part 'type var[,var...];' with a possible C-comment '/* ... */'
while ($decl =~ /^ ?(\w+) ([\w \,]+)\; ?((\/\*.*?\*\/)?)(.*)$/) {
	$type=$1;
	$member=$2;
	$comment=$3;
	$decl=$5;
	while ($member =~ /^(\w+) ?\, ?(.*)$/) {
		push @TYPES,$type;
		push @MEMBERS,$1;
		push @COMMENTS,$comment;
		if ($comment ne '') {
			$comment='/* see above */';
		}
		$member=$2;
	}
	if ($member !~ /^\w+$/) {
		die "illegal struct, must match 'struct name { type var; /*comment*/ ...};': $input_structure";
	}
	push @TYPES,$type;
	push @MEMBERS,$member;
	push @COMMENTS,$comment;
}

if ($decl !~ /^ ?$/) {
	die "illegal struct, must match 'struct name { type var; /*comment*/ ...};': $input_structure";
}

#$STRUCTURE='teststruct';
$STRUCTURE_UC=uc(${STRUCTURE});
#@TYPES=('ex','ex','ex');
#@MEMBERS=('q10','q20','q21');

sub generate {
	my ($template,$conj)=@_;
	my ($res,$N);

	$res='';
	for ($N=1; $N<=$#MEMBERS+1; $N++) {
		$TYPE=$TYPES[$N-1];
		$MEMBER=$MEMBERS[$N-1];
		$COMMENT=$COMMENTS[$N-1];
		$res .= eval('"' . $template . '"');
		$TYPE=''; # to avoid main::TYPE used only once warning
		$MEMBER=''; # same as above
		$COMMENT=''; # same as above
		if ($N!=$#MEMBERS+1) {
			$res .= $conj;
		}
	}
	return $res;
}

$number_of_members=$#MEMBERS+1;
$constructor_arglist=generate('ex tmp_${MEMBER}',', ');
$member_access_functions=generate('    const ex & ${MEMBER}(void) { return m_${MEMBER}; }',"\n");
$op_access_indices_decl=generate('    static unsigned op_${MEMBER};',"\n");
$op_access_indices_def=generate('unsigned ${STRUCTURE}::op_${MEMBER}=${N}-1;',"\n");
$members=generate('    ex m_${MEMBER}; ${COMMENT}',"\n");
$copy_statements=generate('    m_${MEMBER}=other.m_${MEMBER};',"\n");
$constructor_statements=generate('m_${MEMBER}(tmp_${MEMBER})',', ');
$let_op_statements=generate(
'    case ${N}-1:'."\n".
'        return m_${MEMBER};'."\n".
'        break;',
"\n");
$temporary_arglist=generate('tmp_${MEMBER}',', ');
$expand_statements=generate('    ex tmp_${MEMBER}=m_${MEMBER}.expand(options);',"\n");
$has_statements=generate('    if (m_${MEMBER}.has(other)) return true;',"\n");
$eval_statements=generate(
'    ex tmp_${MEMBER}=m_${MEMBER}.eval(level-1);'."\n".
'    all_are_trivially_equal = all_are_trivially_equal &&'."\n".
'                              are_ex_trivially_equal(tmp_${MEMBER},m_${MEMBER});',
"\n");
$evalf_statements=generate(
'    ex tmp_${MEMBER}=m_${MEMBER}.evalf(level-1);'."\n".
'    all_are_trivially_equal = all_are_trivially_equal &&'."\n".
'                              are_ex_trivially_equal(tmp_${MEMBER},m_${MEMBER});',
"\n");
$normal_statements=generate(
'    ex tmp_${MEMBER}=m_${MEMBER}.normal(level-1);'."\n".
'    all_are_trivially_equal = all_are_trivially_equal &&'."\n".
'                              are_ex_trivially_equal(tmp_${MEMBER},m_${MEMBER});',
"\n");
$diff_statements=generate('    ex tmp_${MEMBER}=m_${MEMBER}.diff(s);',"\n");
$subs_statements=generate(
'    ex tmp_${MEMBER}=m_${MEMBER}.subs(ls,lr);'."\n".
'    all_are_trivially_equal = all_are_trivially_equal &&'."\n".
'                              are_ex_trivially_equal(tmp_${MEMBER},m_${MEMBER});',
"\n");
$compare_statements=generate(
'    cmpval=m_${MEMBER}.compare(o.m_${MEMBER});'."\n".
'    if (cmpval!=0) return cmpval;',
"\n");
$is_equal_statements=generate('    if (!m_${MEMBER}.is_equal(o.m_${MEMBER})) return false;',"\n");
$types_ok_statements=generate(
'#ifndef SKIP_TYPE_CHECK_FOR_${TYPE}'."\n".
'    if (!is_exactly_a<${TYPE}>(m_${MEMBER})) return false;'."\n".
'#endif // ndef SKIP_TYPE_CHECK_FOR_${TYPE}',"\n");

$interface=<<END_OF_INTERFACE;
/** \@file ${STRUCTURE}.h
 *
 *  Definition of GiNaC's user defined structure ${STRUCTURE}. */

/*
 *  This file was generated automatically by structure.pl.
 *  Please do not modify it directly, edit the perl script instead!
 *
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

// structure.pl input:
${input_structure}

#ifndef __GINAC_${STRUCTURE_UC}_H__
#define __GINAC_${STRUCTURE_UC}_H__

#include "structure.h"

namespace GiNaC {

class ${STRUCTURE} : public structure
{
// member functions

	// default ctor, dtor, copy ctor, assignment operator and helpers
public:
	${STRUCTURE}();
	~${STRUCTURE}();
	${STRUCTURE}(${STRUCTURE} const & other);
	${STRUCTURE} const & operator=(${STRUCTURE} const & other);
protected:
	void copy(${STRUCTURE} const & other);
	void destroy(bool call_parent);

	// other ctors
public:
	${STRUCTURE}(${constructor_arglist});

	// functions overriding virtual functions from base classes
public:
	basic * duplicate() const;
	void print(const print_context & c, unsigned level = 0) const;
	int nops() const;
	ex & let_op(int i);
	ex expand(unsigned options=0) const;
	bool has(const ex & other) const;
	ex eval(int level=0) const;
	ex evalf(int level=0) const;
	ex normal(lst &sym_lst, lst &repl_lst, int level=0) const;
	ex diff(const symbol & s) const;
	ex subs(const lst & ls, const lst & lr) const;
protected:
	int compare_same_type(const basic & other) const;
	bool is_equal_same_type(const basic & other) const;
	unsigned return_type(void) const;

	// new virtual functions which can be overridden by derived classes
	// none

	// non-virtual functions in this class
public:
${member_access_functions}
	bool types_ok(void) const;
	
// member variables
protected:
${members}
public:
${op_access_indices_decl}
};

// global constants

extern const ${STRUCTURE} some_${STRUCTURE};
extern const type_info & typeid_${STRUCTURE};
extern const unsigned tinfo_${STRUCTURE};

} // namespace GiNaC

#endif // ndef _${STRUCTURE_UC}_H_

END_OF_INTERFACE

$implementation=<<END_OF_IMPLEMENTATION;
/** \@file ${STRUCTURE}.cpp
 *
 *  Implementation of GiNaC's user defined structure ${STRUCTURE}. */

/*
 *  This file was generated automatically by STRUCTURE.pl.
 *  Please do not modify it directly, edit the perl script instead!
 *
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

// structure.pl input:
${input_structure}

#include <iostream>
#include <stdexcept>

#include "${STRUCTURE}.h"
#include "print.h"

namespace GiNaC {

//////////
// default ctor, destructor, copy ctor assignment operator and helpers
//////////

// public

${STRUCTURE}::${STRUCTURE}()
{
	tinfo_key=tinfo_${STRUCTURE};
}

${STRUCTURE}::~${STRUCTURE}()
{
	destroy(false);
}

${STRUCTURE}::${STRUCTURE}(${STRUCTURE} const & other)
{
	copy(other);
}

${STRUCTURE} const & ${STRUCTURE}::operator=(${STRUCTURE} const & other)
{
	if (this != &other) {
		destroy(true);
		copy(other);
	}
	return *this;
}

// protected

void ${STRUCTURE}::copy(${STRUCTURE} const & other)
{
	structure::copy(other);
${copy_statements}
}

void ${STRUCTURE}::destroy(bool call_parent)
{
	if (call_parent) structure::destroy(call_parent);
}

//////////
// other ctors
//////////

// public

${STRUCTURE}::${STRUCTURE}(${constructor_arglist}) 
	: ${constructor_statements}
{
	tinfo_key=tinfo_${STRUCTURE};
}

//////////
// functions overriding virtual functions from base classes
//////////

// public

basic * ${STRUCTURE}::duplicate() const
{
	return new ${STRUCTURE}(*this);
}

void ${STRUCTURE}::print(const print_context & c, unsigned level) const
{
	c.s << class_name() << "()";
}

int ${STRUCTURE}::nops() const
{
	return ${number_of_members};
}

ex & ${STRUCTURE}::let_op(int i)
{
	GINAC_ASSERT(i>=0);
	GINAC_ASSERT(i<nops());

	switch (i) {
${let_op_statements}
	}
	throw(std::runtime_error("${STRUCTURE}::let_op(): should not have reached this point"));
}

ex ${STRUCTURE}::expand(unsigned options) const
{
	bool all_are_trivially_equal=true;
${expand_statements}
	if (all_are_trivially_equal) {
		return *this;
	}
	return ${STRUCTURE}(${temporary_arglist});
}

// a ${STRUCTURE} 'has' an expression if it is this expression itself or a child 'has' it

bool ${STRUCTURE}::has(const ex & other) const
{
	GINAC_ASSERT(other.bp!=0);
	if (is_equal(*other.bp)) return true;
${has_statements}
	return false;
}

ex ${STRUCTURE}::eval(int level) const
{
	if (level==1) {
		return this->hold();
	}
	bool all_are_trivially_equal=true;
${eval_statements}
	if (all_are_trivially_equal) {
		return this->hold();
	}
	return ${STRUCTURE}(${temporary_arglist});
}

ex ${STRUCTURE}::evalf(int level) const
{
	if (level==1) {
		return *this;
	}
	bool all_are_trivially_equal=true;
${evalf_statements}
	if (all_are_trivially_equal) {
		return *this;
	}
	return ${STRUCTURE}(${temporary_arglist});
}

/** Implementation of ex::normal() for ${STRUCTURE}s. It normalizes the arguments
 *  and replaces the ${STRUCTURE} by a temporary symbol.
 *  \@see ex::normal */
ex ${STRUCTURE}::normal(lst &sym_lst, lst &repl_lst, int level) const
{
	if (level==1) {
		return basic::normal(sym_lst,repl_lst,level);
	}
	bool all_are_trivially_equal=true;
${normal_statements}
	if (all_are_trivially_equal) {
		return basic::normal(sym_lst,repl_lst,level);
	}
	ex n=${STRUCTURE}(${temporary_arglist});
	return n.bp->basic::normal(sym_lst,repl_lst,level);
}

/** ${STRUCTURE}::diff() differentiates the children.
	there is no need to check for triavially equal, since diff usually
	does not return itself unevaluated. */
ex ${STRUCTURE}::diff(const symbol & s) const
{
${diff_statements}
	return ${STRUCTURE}(${temporary_arglist});
}

ex ${STRUCTURE}::subs(const lst & ls, const lst & lr) const
{
	bool all_are_trivially_equal=true;
${subs_statements}
	if (all_are_trivially_equal) {
		return *this;
	}
	return ${STRUCTURE}(${temporary_arglist});
}

// protected

int ${STRUCTURE}::compare_same_type(const basic & other) const
{
	GINAC_ASSERT(is_a<${STRUCTURE}>(other));
	${STRUCTURE} const &o = static_cast<const ${STRUCTURE} &>(other);
	int cmpval;
${compare_statements}
	return 0;
}

bool ${STRUCTURE}::is_equal_same_type(const basic & other) const
{
	GINAC_ASSERT(is_a<${STRUCTURE}>(other));
	${STRUCTURE} const &o = static_cast<const ${STRUCTURE} &>(other);
${is_equal_statements}
	return true;
}

unsigned ${STRUCTURE}::return_type(void) const
{
	return return_types::noncommutative_composite;
}

//////////
// new virtual functions which can be overridden by derived classes
//////////

// none

//////////
// non-virtual functions in this class
//////////

// public

#define SKIP_TYPE_CHECK_FOR_ex
// this is a hack since there is no meaningful is_a<ex>(...) definition

bool ${STRUCTURE}::types_ok(void) const
{
${types_ok_statements}
	return true;
}

//////////
// static member variables
//////////

${op_access_indices_def}

//////////
// global constants
//////////

const ${STRUCTURE} some_${STRUCTURE};
const type_info & typeid_${STRUCTURE}=typeid(some_${STRUCTURE});
const unsigned tinfo_${STRUCTURE}=structure::register_new("${STRUCTURE}");

} // namespace GiNaC

END_OF_IMPLEMENTATION

print "Creating interface file ${STRUCTURE}.h...";
open OUT,">${STRUCTURE}.h" or die "cannot open ${STRUCTURE}.h";
print OUT $interface;
close OUT;
print "ok.\n";

print "Creating implementation file ${STRUCTURE}.cpp...";
open OUT,">${STRUCTURE}.cpp" or die "cannot open ${STRUCTURE}.cpp";
print OUT $implementation;
close OUT;
print "ok.\n";

print "done.\n";
