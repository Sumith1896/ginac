/** @file print.cpp
 *
 *  Implementation of helper classes for expression output. */

/*
 *  GiNaC Copyright (C) 1999-2001 Johannes Gutenberg University Mainz, Germany
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

#include "print.h"

namespace GiNaC {

print_context::print_context()
	: s(std::cout) {}
print_context::print_context(std::ostream & os)
	: s(os) {}

print_latex::print_latex()
	: print_context(std::cout) {}
print_latex::print_latex(std::ostream & os)
	: print_context(os) {}

print_tree::print_tree(unsigned d)
	: print_context(std::cout), delta_indent(d) {}
print_tree::print_tree(std::ostream & os, unsigned d)
	: print_context(os), delta_indent(d) {}

print_csrc::print_csrc()
	: print_context(std::cout) {}
print_csrc::print_csrc(std::ostream & os)
	: print_context(os) {}

print_csrc_float::print_csrc_float()
	: print_csrc(std::cout) {}
print_csrc_float::print_csrc_float(std::ostream & os)
	: print_csrc(os) {}

print_csrc_double::print_csrc_double()
	: print_csrc(std::cout) {}
print_csrc_double::print_csrc_double(std::ostream & os)
	: print_csrc(os) {}

print_csrc_cl_N::print_csrc_cl_N()
	: print_csrc(std::cout) {}
print_csrc_cl_N::print_csrc_cl_N(std::ostream & os)
	: print_csrc(os) {}

} // namespace GiNaC