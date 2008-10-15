#include <stdexcept>
#include <sstream>
#include "parser.hpp"
#include "lexer.hpp"
#include "debug.hpp"
#include "mul.h"
#include "constant.h"

namespace GiNaC 
{

/// identifier_expr:  identifier |  identifier '(' expression* ')'
ex parser::parse_identifier_expr()
{
	std::string name = scanner->str;
	get_next_tok();  // eat identifier.

	if (token != '(') // symbol
		return find_or_insert_symbol(name, syms, strict);

	// function/ctor call.
	get_next_tok();  // eat (
	exvector args;
	if (token != ')') {
		while (true) {
			ex e = parse_expression();
			args.push_back(e);

			if (token == ')')
				break;

			if (token != ',')
				Parse_error("expected ')' or ',' in argument list");

			get_next_tok();
		}
	}
	// Eat the ')'.
	get_next_tok();
	prototype the_prototype = make_pair(name, args.size());
	prototype_table::const_iterator reader = funcs.find(the_prototype);
	if (reader == funcs.end()) {
		Parse_error_("no function \"" << name << "\" with " <<
			     args.size() << " arguments");
	}
	ex ret = reader->second(args);
	return ret;
}

/// paren_expr:  '(' expression ')'
ex parser::parse_paren_expr()
{
	get_next_tok();  // eat (.
	ex e = parse_expression();

	if (token != ')')
		Parse_error("expected ')'");
	get_next_tok();  // eat ).
	return e;
}

extern numeric* _num_1_p;
extern ex _ex0;

/// unary_expr: [+-] expression
ex parser::parse_unary_expr()
{
	// Unlike most other parse_* method this one does NOT consume
	// current token so parse_binop_rhs() knows what kind of operator
	// is being parsed.
	
	// There are different kinds of expressions which need to be handled:
	// -a+b 
	// -(a) 
	// +a
	// +(a)
	// Delegete the work to parse_binop_rhs(), otherwise we end up
	// duplicating it here. 
	ex lhs = _ex0; // silly trick
	ex e = parse_binop_rhs(0, lhs);
	return e;
}

/// primary: identifier_expr | number_expr | paren_expr | unary_expr 
ex parser::parse_primary() 
{
	switch (token) {
		case lexer::token_type::identifier:
			 return parse_identifier_expr();
		case lexer::token_type::number:
			 return parse_number_expr();
		case '(': 
			 return parse_paren_expr();
		case '-':
		case '+':
			 return parse_unary_expr();
		case lexer::token_type::literal:
			 return parse_literal_expr();
		case lexer::token_type::eof:
		default:
			 Parse_error("unexpected token");
	}
}

/// expression ::= primary binoprhs
ex parser::parse_expression() 
{
	ex lhs = parse_primary();
	ex res = parse_binop_rhs(0, lhs);
	return res;
}

/// number_expr: number
ex parser::parse_number_expr()
{
	ex n = numeric(scanner->str.c_str());
	get_next_tok(); // consume the number
	return n;
}

/// literal_expr: 'I' | 'Pi' | 'Euler' | 'Catalan'
ex parser::parse_literal_expr()
{
	get_next_tok(); // consume the literal
	if (scanner->str == "I")
		return I;
	else if (scanner->str == "Pi")
		return Pi;
	else if (scanner->str == "Euler")
		return Euler;
	else if (scanner->str == "Catalan")
		return Catalan;
	bug("unknown literal: \"" << scanner->str << "\"");
}

ex parser::operator()(std::istream& input)
{
	scanner->switch_input(&input);
	get_next_tok();
	ex ret = parse_expression();
	// parse_expression() stops if it encounters an unknown token.
	// This is not a bug: since the parser is recursive checking
	// whether the next token is valid is responsibility of the caller.
	// Hence make sure nothing is left in the stream:
	if (token != lexer::token_type::eof)
		Parse_error("expected EOF");

	return ret;
}

ex parser::operator()(const std::string& input)
{
	std::istringstream is(input);
	ex ret = operator()(is);
	return ret;
}

int parser::get_next_tok()
{
	token = scanner->gettok();
	return token;
}

parser::parser(const symtab& syms_, const bool strict_,
	       const prototype_table& funcs_) : strict(strict_),
	funcs(funcs_), syms(syms_)
{
	scanner = new lexer();
}

parser::~parser()
{
	delete scanner;
}

} // namespace GiNaC