//============================================================================
// Name        : NovaUtil.h
// Copyright   : DataSoft Corporation 2011-2012
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Utility functions that are used by multiple components of nova
//					but do not warrant an object
//============================================================================/*

#ifndef NOVAUTIL_H_
#define NOVAUTIL_H_

#include <string>
#include <vector>

#include "sys/types.h"

namespace Nova{

// Encrpyts/decrypts a char buffer of size 'size' depending on mode
// TODO: Comment more on this once it's written
void CryptBuffer(u_char *buf, uint size, bool mode);

// Gets local IP address for interface
//		dev - Device name, e.g. "eth0"
// Returns: IP addresses
std::string GetLocalIP(const char *dev);

//Removes any instance of the specified character from the front and back of the string
//		str - pointer to the string you want to modify
// 		c - character you wish to remove (Whitespace by default)
// Note: this function will result in an empty string, if every character is == c
void Trim(std::string& str, char c = ' ');

//Replaces all instances of the search character in the addressed string with the character specified
//	str: the string to modify
//	searchChar: the character to replace
//	replaceVal: the replacement character
void ReplaceChar(std::string& str, char searchChar, char replaceVal);

//Takes the input vector of doubles, and changes the double at the index to the target value, then shifts
// all other values in the vector proportionally such that the entire vector sums to 100
//	inputDoubles: The entire list of doubles
//	targetIndex: The index in the vector to change, defaults to 0 if not provided
//	targetValue: The value to shift the double at targetIndex to.
//**Note**: if inputDoulbes does not sum to 100, it will be modified so that it does before calculation
//	ie. ShiftDistribution(vector<4, 4, 2>, 30, 2) will pre-calc the vector<40,40,20> and return <35, 35, 30>
std::vector<double> ShiftDistribution(std::vector<double> inputDoubles, double targetValue, int targetIndex = 0);

//This function takes the vector of doubles and rounds them to integer values preserving the integer sum of the vector
// ie. if this were a vector of percentages <33.333, 33.333, 33.333> it would return <33,34,33>
//	inputDoulbes: The list of doubles to round
std::vector<int> RoundDistributionToIntegers(std::vector<double> inputDoubles);

}

#endif /* NOVAUTIL_H_ */
