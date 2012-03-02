//============================================================================
// Name        : StatusQueries.h
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
// Description : Controls the Novad process itself, in terms of stopping and starting
//============================================================================

#include "NovadControl.h"
#include "messages/UI_Message.h"
#include "messages/ControlMessage.h"

using namespace Nova;

bool Nova::StartNovad()
{
	//TODO: Obviously, fill this out to do something
	return false;
}

bool Nova::StopNovad()
{
	ControlMessage *killRequest = new ControlMessage();
	killRequest->m_controlType = CONTROL_EXIT_REQUEST;
	if( UI_Message::WriteMessage(killRequest, 0) ) //TODO: Change this to a real socket
	{
		//There was an error in sending the message
		//TODO: Log this fact
		return false;
	}

	UI_Message *reply = UI_Message::ReadMessage(0); //TODO: Change this to a real socket
	if( reply == NULL )
	{
		//There was an error receiving the reply
		//TODO: Log this fact
		return false;
	}
	ControlMessage *killReply = (ControlMessage*)reply;
	if( killReply->m_controlType != CONTROL_EXIT_REPLY )
	{
		//Received the wrong kind of message
		return false;
	}
	return killReply->m_success;

}