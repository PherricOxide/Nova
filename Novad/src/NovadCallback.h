//============================================================================
// Name        : NovadCallback.h
// Copyright   : DataSoft Corporation 2011-2013
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
// Description : Child class of ServerCallback. Specifies a function to be run
//		as the callback thread for incoming connections
//============================================================================

#ifndef NOVADCALLBACK_H_
#define NOVADCALLBACK_H_

#include "messaging/ServerCallback.h"

namespace Nova
{

class NovadCallback : public ServerCallback
{

protected:

	void CallbackThread(int socketFD);
};

}

#endif /* NOVADCALLBACK_H_ */
