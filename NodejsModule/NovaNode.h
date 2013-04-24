//============================================================================
// Name        : NovaNode.h
// Copyright   : DataSoft Corporation 2011-2013
//      Nova is free software: you can redistribute it and/or modify
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
// Description : Exposes Nova_UI_Core as a module for the node.js environment.
//============================================================================

#ifndef NOVANODE_H
#define NOVANODE_H

#include <v8.h>
#include <node.h>


/* Nova headers */
#include "nova_ui_core.h"
#include "NovaUtil.h"
#include "Suspect.h"
#include "Logger.h"

/* NovaNode headers */
#include "v8Helper.h"
#include "SuspectTable.h"

using namespace node;
using namespace v8;
using namespace Nova;
using namespace std;

class NovaNode: ObjectWrap
{
private:
	int m_count;
	static pthread_t m_NovaCallbackThread;
	static bool m_NovaCallbackHandlingContinue;
	static Persistent<Function> m_CallbackFunction;
	static Persistent<Function> m_SuspectsClearedCallback;
	static Persistent<Function> m_SuspectClearedCallback;
	static bool m_CallbackRegistered;
	static bool m_AllSuspectsClearedCallbackRegistered;
	static bool m_SuspectClearedCallbackRegistered;
	static bool m_callbackRunning;
	static SuspectHashTable m_suspects;

	static void InitNovaCallbackProcessing();
	static void CheckInitNova();

	static void NovaCallbackHandling(eio_req __attribute__((__unused__)) *req);
	static int AfterNovaCallbackHandling(eio_req __attribute__((__unused__)) *req);
	static void HandleAllSuspectsCleared();
	static void HandleSuspectCleared(Suspect *);
	static int HandleAllClearedOnV8Thread(eio_req *);
	static int HandleMessageWithIDOnV8Thread(eio_req *);
	static int HandleSuspectClearedOnV8Thread(eio_req *);
	static void HandleCallbackError();

public:

	static Persistent<FunctionTemplate> s_ct;

	static void Init(Handle<Object> target);
	static Handle<Value> CheckConnection(const Arguments __attribute__((__unused__)) &args);
	static Handle<Value> Shutdown(const Arguments __attribute__((__unused__)) &args);
	static Handle<Value> ClearSuspect(const Arguments &args);
	NovaNode();
	~NovaNode();

	static void SynchInternalList();
	static void DoneWithSuspectCallback(Persistent<Value> suspect, void *paramater);
	static Handle<Value> New(const Arguments& args);
	static Handle<Value> GetFeatureNames(const Arguments& args);
	static Handle<Value> GetDIM(const Arguments& args);
	static Handle<Value> GetSupportedEngines(const Arguments& args);
	static Handle<Value> RequestSuspectCallback(const Arguments& args);
	static Handle<Value> ClearAllSuspects(const Arguments& args);
	static Handle<Value> registerOnNewSuspect(const Arguments& args);
	static Handle<Value> registerOnAllSuspectsCleared(const Arguments& args);
	static Handle<Value> registerOnSuspectCleared(const Arguments& args);
	static Handle<Value> RequestSuspectDetailsString(const Arguments& args);
	static void HandleOnNewSuspectWeakCollect(Persistent<Value> __attribute__((__unused__)) OnNewSuspectCallback, void __attribute__((__unused__)) * parameter);
};

#endif
