//============================================================================
// Name        : MessageQueue.h
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
// Description : An item in the MessageManager's table. Contains a queue of received
//	messages on a particular socket
//============================================================================

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include "messages/UI_Message.h"

#include "pthread.h"
#include <queue>

namespace Nova
{

class MessageQueue
{
public:

	MessageQueue(int socketFD, pthread_t callbackHandler);

	//Will block and wait until the queue has been flushed by calls to PopMessage
	~MessageQueue();

	//blocking call
	UI_Message *PopMessage();

private:

	//Producer thread, adds to queue
	static void *StaticThreadHelper(void *ptr);

	void PushMessage(UI_Message *message);

	void *ProducerThread();

	std::queue<UI_Message*> m_messages;
	int m_socket;

	pthread_cond_t m_wakeupCondition;
	pthread_t m_callbackThread;
	pthread_t m_producerThread;
	pthread_mutex_t m_queueMutex;

};


}


#endif /* MESSAGEQUEUE_H_ */
