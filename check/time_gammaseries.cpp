/** @file time_gammaseries.cpp
 *
 *  Some timings on series expansion of the Gamma function around a pole. */

/*
 *  GiNaC Copyright (C) 1999-2004 Johannes Gutenberg University Mainz, Germany
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

#include "times.h"

unsigned tgammaseries(unsigned order)
{
	unsigned result = 0;
	symbol x;

	ex myseries = series(tgamma(x),x==0,order);
	// compute the last coefficient numerically:
	ex last_coeff = myseries.coeff(x,order-1).evalf();
	// compute a bound for that coefficient using a variation of the leading
	// term in Stirling's formula:
	ex bound = exp(-.57721566490153286*(order-1))/(order-1);
	if (abs((last_coeff-pow(-1,order))/bound) > 1) {
		clog << "The " << order-1
		     << "th order coefficient in the power series expansion of tgamma(0) was erroneously found to be "
		     << last_coeff << ", violating a simple estimate." << endl;
		++result;
	}

	return result;
}

unsigned time_gammaseries(void)
{
	unsigned result = 0;

	cout << "timing Laurent series expansion of Gamma function" << flush;
	clog << "-------Laurent series expansion of Gamma function:" << endl;

	vector<unsigned> sizes;
	vector<double> times;
	timer omega;

	sizes.push_back(10);
	sizes.push_back(15);
	sizes.push_back(20);
	sizes.push_back(25);

	for (vector<unsigned>::iterator i=sizes.begin(); i!=sizes.end(); ++i) {
		omega.start();
		result += tgammaseries(*i);
		times.push_back(omega.read());
		cout << '.' << flush;
	}

	if (!result) {
		cout << " passed ";
		clog << "(no output)" << endl;
	} else {
		cout << " failed ";
	}
	// print the report:
	cout << endl << "	order: ";
	for (vector<unsigned>::iterator i=sizes.begin(); i!=sizes.end(); ++i)
		cout << '\t' << (*i);
	cout << endl << "	time/s:";
	for (vector<double>::iterator i=times.begin(); i!=times.end(); ++i)
		cout << '\t' << int(1000*(*i))*0.001;
	cout << endl;
	
	return result;
}
