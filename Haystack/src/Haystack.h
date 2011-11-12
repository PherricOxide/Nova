//============================================================================
// Name        : Haystack.h
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Nova utility for transforming Honeyd log files into
//					TrafficEvents usable by Nova's Classification Engine.
//============================================================================

#ifndef HAYSTACK_H_
#define HAYSTACK_H_

///	Filename of the file to be used as an IPC key
#define KEY_FILENAME "/keys/NovaIPCKey"
/// File name of the file to be used as GUI Input IPC key.
#define GUI_FILENAME "/keys/GUI_HSKey"
//Number of values read from the NOVAConfig file
#define CONFIG_FILE_LINE_COUNT 8
///The maximum message, as defined in /proc/sys/kernel/msgmax
#define MAX_MSG_SIZE 65535
//Number of messages to queue in a listening socket before ignoring requests until the queue is open
#define SOCKET_QUEUE_SIZE 50

#include <TrafficEvent.h>

namespace Nova{
namespace Haystack{

//Loads configuration variables from NOVAConfig_HS.txt or specified config file
void LoadConfig(char* input);

using namespace std;

/// Thread for listening for GUI commands
void *GUILoop(void *ptr);

/// Receives input commands from the GUI
void ReceiveGUICommand(int socket);

/// Callback function that is passed to pcap_loop(..) and called each time
/// a packet is recieved
void Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet);

/// Thread for periodically checking for TCP timeout.
///	IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
///	This thread looks for old tcp sessions and declares them terminated
void *TCPTimeout( void *ptr );

///Sends the given TrafficEvent to the Classification Engine
///	Returns success or failure
bool SendToCE( TrafficEvent *event );

///Parse through the honeyd config file and get the list of IP addresses used
vector <string> GetHaystackAddresses(string honeyDConfigPath);

///Usage tips
string Usage();

///Hash table for current TCP Sessions
///Table key is the source network socket, comprised of IP and Port in string format
///	IE: "192.168.1.1-8080"

struct Session
{
	bool fin;
	vector<struct Packet> session;
};

///The Value is a vector of IP headers
typedef std::tr1::unordered_map<string, struct Session> TCPSessionHashTable;
}
}

#endif /* HAYSTACK_H_ */
