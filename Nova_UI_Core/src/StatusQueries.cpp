//============================================================================
// Name        : StatusQueries.cpp
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
// Description : Handles requests for information from Novad
//============================================================================

#include "Connection.h"
#include "StatusQueries.h"
#include "messages/ControlMessage.h"
#include "messages/RequestMessage.h"
#include "messages/ErrorMessage.h"

#include <iostream>

extern int novadListenSocket;
using namespace Nova;
using namespace std;

bool Nova::IsNovadUp(bool tryToConnect)
{

	if(tryToConnect)
	{
		//If we couldn't connect, then it's definitely not up
		if(!ConnectToNovad())
		{
			return false;
		}
	}

	ControlMessage ping(CONTROL_PING);
	if(!UI_Message::WriteMessage(&ping, novadListenSocket) )
	{
		//There was an error in sending the message
		return false;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket);
	if(reply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)reply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != CONTROL_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return false;
	}

	ControlMessage *pong = (ControlMessage*)reply;
	if(pong->m_controlType != CONTROL_PONG)
	{
		//Received the wrong kind of control message
		delete pong;
		return false;
	}
	delete pong;
	return true;
}

vector<in_addr_t> *Nova::GetSuspectList(enum SuspectListType listType)
{
	RequestMessage request(REQUEST_SUSPECTLIST);
	request.m_listType = listType;

	if(!UI_Message::WriteMessage(&request, novadListenSocket) )
	{
		//There was an error in sending the message
		return NULL;
	}

	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket);
	if(reply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)reply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != REQUEST_MESSAGE )
	{
		//Received the wrong kind of message
		delete reply;
		return NULL;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_requestType != REQUEST_SUSPECTLIST_REPLY)
	{
		//Received the wrong kind of control message
		delete requestReply;
		return NULL;
	}


	vector<in_addr_t> *ret = new vector<in_addr_t>;
	*ret = requestReply->m_suspectList;

	delete requestReply;
	return ret;
}

Suspect *Nova::GetSuspect(in_addr_t address)
{
	RequestMessage request(REQUEST_SUSPECT);
	request.m_suspectAddress = address;


	if(!UI_Message::WriteMessage(&request, novadListenSocket) )
	{
		//There was an error in sending the message
		return NULL;
	}


	UI_Message *reply = UI_Message::ReadMessage(novadListenSocket);
	if(reply->m_messageType == ERROR_MESSAGE )
	{
		ErrorMessage *error = (ErrorMessage*)reply;
		if(error->m_errorType == ERROR_SOCKET_CLOSED)
		{
			CloseNovadConnection();
		}
		delete error;
		return false;
	}
	if(reply->m_messageType != REQUEST_MESSAGE)
	{
		//Received the wrong kind of message
		delete reply;
		return NULL;
	}

	RequestMessage *requestReply = (RequestMessage*)reply;
	if(requestReply->m_requestType != REQUEST_SUSPECT_REPLY)
	{
		//Received the wrong kind of control message
		delete requestReply;
		return NULL;
	}

	Suspect *returnSuspect = new Suspect();
	*returnSuspect = *requestReply->m_suspect;
	delete requestReply;

	return returnSuspect;
}
