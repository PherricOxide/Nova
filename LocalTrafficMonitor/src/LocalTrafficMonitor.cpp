//============================================================================
// Name        : LocalTrafficMonitor.cpp
// Author      : DataSoft Corporation
// Copyright   : GNU GPL v3
// Description : Monitors local network traffic and sends detailed TrafficEvents
//					to the Classification Engine.
//============================================================================/*

#include <errno.h>
#include <arpa/inet.h>
#include <TrafficEvent.h>
#include <fstream>
#include "LocalTrafficMonitor.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/un.h>
#include <log4cxx/xml/domconfigurator.h>
#include <boost/archive/text_oarchive.hpp>


using namespace log4cxx;
using namespace log4cxx::xml;
using namespace std;
using namespace Nova;

int tcpTime; //TCP_TIMEOUT Measured in seconds
int tcpFreq; //TCP_CHECK_FREQ Measured in seconds
static LocalTrafficMonitor::TCPSessionHashTable SessionTable;

string dev; //Interface name, read from config file

pthread_rwlock_t lock;
LoggerPtr m_logger(Logger::getLogger("main"));

/// Callback function that is passed to pcap_loop(..) and called each time
/// a packet is recieved
void Nova::LocalTrafficMonitor::Packet_Handler(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char* packet)
{
	struct ether_header *ethernet;  	/* net/ethernet.h */
		struct ip *ip_hdr; 					/* The IP header */

		TrafficEvent *event;
		struct Packet temp;

		if(packet == NULL)
		{
			LOG4CXX_ERROR(m_logger, "Didn't grab packet!");
			return;
		}

		/* let's start with the ether header... */
		ethernet = (struct ether_header *) packet;

		/* Do a couple of checks to see what packet type we have..*/
		if (ntohs (ethernet->ether_type) == ETHERTYPE_IP)
		{
			ip_hdr = (struct ip*)(packet + sizeof(struct ether_header));

			//Prepare Packet structure
			struct Packet *packet_info = &temp;
			packet_info->ip_hdr = *ip_hdr;
			packet_info->pcap_header = *pkthdr;

			//IF UDP or ICMP
			if(ip_hdr->ip_p == 17 )
			{
				packet_info->udp_hdr = *(struct udphdr*) ((char *)ip_hdr + sizeof(struct ip));
				event = new Nova::TrafficEvent(*packet_info, FROM_HAYSTACK_DP);
				SendToCE(event);
				delete event;
				event = NULL;
			}
			else if(ip_hdr->ip_p == 1)
			{
				packet_info->icmp_hdr = *(struct icmphdr*) ((char *)ip_hdr + sizeof(struct ip));
				event = new Nova::TrafficEvent(*packet_info, FROM_HAYSTACK_DP);
				SendToCE(event);
				delete event;
				event = NULL;
			}
			//If TCP...
			else if(ip_hdr->ip_p == 6)
			{
				packet_info->tcp_hdr = *(struct tcphdr*)((char*)ip_hdr + sizeof(struct ip));
				int dest_port = ntohs(packet_info->tcp_hdr.dest);
				char tcp_socket[55];

				bzero(tcp_socket, 55);
				snprintf(tcp_socket, 55, "%d-%d-%d", ip_hdr->ip_dst.s_addr, ip_hdr->ip_src.s_addr, dest_port);
				pthread_rwlock_wrlock(&lock);

				//If this is a new entry...
				if( SessionTable[tcp_socket].session.size() == 0)
				{
					//Insert packet into Hash Table
					SessionTable[tcp_socket].session.push_back(*packet_info);
					SessionTable[tcp_socket].fin = false;
				}

				//If there is already a session in progress for the given LogEntry
				else
				{
					//If Session is ending
					//TODO: The session may continue a few packets after the FIN. Account for this case.
					//See ticket #15
					if(packet_info->tcp_hdr.fin)
					{
						SessionTable[tcp_socket].session.push_back(*packet_info);
						SessionTable[tcp_socket].fin = true;
					}
					else
					{
						//Add this new packet to the session vector
						SessionTable[tcp_socket].session.push_back(*packet_info);
					}
				}
				pthread_rwlock_unlock(&lock);
			}
		}
		else if(ntohs(ethernet->ether_type) == ETHERTYPE_ARP)
		{
			return;
		}
		else
		{
			LOG4CXX_ERROR(m_logger, "Unknown Non-IP Packet Received");
			return;
		}
}

int main(int argc, char *argv[])
{
	using namespace LocalTrafficMonitor;
	pthread_rwlock_init(&lock, NULL);
	pthread_t TCP_timeout_thread;

	char errbuf[PCAP_ERRBUF_SIZE];
	int ret;

	//Path name variable for config file, set to a default
	char* nConfig = (char*)"Config/NOVAConfig_LTM.txt";
	string line; //used for input checking

	bpf_u_int32 maskp;				/* subnet mask */
	bpf_u_int32 netp; 				/* ip          */

	string hostAddress;
	int c;

	while((c = getopt (argc, argv, ":n:l:")) != -1)
	{
		switch(c)
		{

			//"NOVA Config"
			case 'n':
				if(optarg != NULL)
				{
					line = string(optarg);
					if(line.size() > 4 && !line.substr(line.size()-4, line.size()).compare(".txt"))
					{
						nConfig = (char *)optarg;
					}
				}
				else
				{
					cerr << "Bad Input File Path" << endl;
					cout << Usage();
					exit(1);
				}
				break;

			//Log config file
			case 'l':
				if(optarg != NULL)
				{
					line = string(optarg);
					if(line.size() > 4 && !line.substr(line.size()-4, line.size()).compare(".xml"))
					{
						DOMConfigurator::configure(optarg);
					}
				}
				else
				{
					cerr << "Bad Output File Path" << endl;
					cout << Usage();
					exit(1);
				}
				break;

			case '?':
				cerr << "You entered an unrecognized input flag: " << (char)optopt << endl;
				cout << Usage();
				exit(1);
				break;

			case ':':
				cerr << "You're missing an argument after the flag: " << (char)optopt << endl;
				cout << Usage();
				exit(1);
				break;

			default:
			{
				cerr << "Sorry, I didn't recognize the option: " << (char)c << endl;
				cout << Usage();
				exit(1);
			}
		}
	}

	//Runs the configuration loader
	LoadConfig(nConfig);

	struct bpf_program fp;			/* The compiled filter expression */
	char filter_exp[64];
	pcap_t *handle;

	//Open in non-promiscuous mode, since we only want traffic destined for the host machine
	handle = pcap_open_live(dev.c_str(), BUFSIZ, 0, 1000, errbuf);

	if(handle == NULL)
	{
		fprintf(stderr, "Couldn't open device %s: %s\n", dev.c_str(), errbuf);
		return(2);
	}

	/* ask pcap for the network address and mask of the device */
	ret = pcap_lookupnet(dev.c_str(), &netp, &maskp, errbuf);

	if(ret == -1)
	{
		printf("%s\n",errbuf);
		exit(1);
	}

	hostAddress = getLocalIP(dev.c_str());

	if(hostAddress.empty())
	{
		LOG4CXX_ERROR(m_logger, "Invalid interface given");
		exit(1);
	}

	//Form the Filter Expression String
	bzero(filter_exp, 64);
	snprintf(filter_exp, 64, "dst host %s", hostAddress.data());
	LOG4CXX_INFO(m_logger,  filter_exp);

	if (pcap_compile(handle, &fp, filter_exp, 0, maskp) == -1)
	{
		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}

	if (pcap_setfilter(handle, &fp) == -1)
	{
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}

	pthread_create(&TCP_timeout_thread, NULL, TCPTimeout, NULL);

	//"Main Loop"
	//Runs the function "Packet_Handler" every time a packet is received
    pcap_loop(handle, -1, Packet_Handler, NULL);
	return 0;
}

/// Thread for periodically checking for TCP timeout.
///	IE: Not all TCP sessions get torn down properly. Sometimes they just end midstram
///	This thread looks for old tcp sessions and declares them terminated
void *Nova::LocalTrafficMonitor::TCPTimeout( void *ptr )
{
	while(1)
	{
		//Check only once every TCP_CHECK_FREQ seconds
		sleep(tcpFreq);
		time_t currentTime = time(NULL);
		time_t packetTime;

		pthread_rwlock_rdlock(&lock);
		for ( TCPSessionHashTable::iterator it = SessionTable.begin() ; it != SessionTable.end(); it++ )
		{
			if(it->second.session.size() > 0)
			{
				packetTime = it->second.session.back().pcap_header.ts.tv_sec;
				// If it exists)
				if(packetTime + 2 < currentTime)
				{
					//If session has been finished for more than two seconds
					if(it->second.fin == true)
					{
						TrafficEvent *event = new TrafficEvent( &(SessionTable[it->first].session), FROM_LTM);
						SendToCE(event);

						pthread_rwlock_unlock(&lock);
						pthread_rwlock_wrlock(&lock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&lock);
						pthread_rwlock_rdlock(&lock);

						delete event;
						event = NULL;
					}
					//If this session is timed out
					else if(packetTime + tcpTime < currentTime)
					{

						TrafficEvent *event = new TrafficEvent( &(SessionTable[it->first].session), FROM_LTM);
						SendToCE(event);

						pthread_rwlock_unlock(&lock);
						pthread_rwlock_wrlock(&lock);
						SessionTable[it->first].session.clear();
						SessionTable[it->first].fin = false;
						pthread_rwlock_unlock(&lock);
						pthread_rwlock_rdlock(&lock);

						delete event;
						event = NULL;
					}
				}
			}
		}
		pthread_rwlock_unlock(&lock);
	}
	//Shouldn't get here
	LOG4CXX_ERROR(m_logger, "TCP Timeout Thread has halted!");
	return NULL;
}

///Sends the given TrafficEvent to the Classification Engine
///	Returns success or failure
bool Nova::LocalTrafficMonitor::SendToCE( TrafficEvent *event )
{
	stringstream ss;
	boost::archive::text_oarchive oa(ss);

	int socketFD, len;
	struct sockaddr_un remote;

	//Serialize the data into a simple char buffer
	oa << event;
	string temp = ss.str();

	const char* data = temp.c_str();
	int dataLen = temp.size();

	if((socketFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		close(socketFD);
		return false;
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, KEY_FILENAME);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if(connect(socketFD, (struct sockaddr *)&remote, len) == -1)
	{
		perror("connect");
		close(socketFD);
		return false;
	}
	if(send(socketFD, data, dataLen, 0) == -1)
	{
		perror("send");
		close(socketFD);
		return false;
	}
	close(socketFD);
    return true;
}

///Returns a string representation of the specified device's IP address
string Nova::LocalTrafficMonitor::getLocalIP(const char *dev)
{
	static struct ifreq ifreqs[20];
	struct ifconf ifconf;
	uint nifaces, i;

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	int sock, rval;
	sock = socket(AF_INET,SOCK_STREAM,0);

	if(sock < 0)
	{
		perror("socket");
		close(sock);
		return (NULL);
	}

	if((rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf  )) < 0)
	{
		perror("ioctl(SIOGIFCONF)");
	}

	close(sock);
	nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

	for(i = 0; i < nifaces; i++)
	{
		if( strcmp(ifreqs[i].ifr_name, dev) == 0 )
		{
			char ip_addr [ INET_ADDRSTRLEN ];
			struct sockaddr_in *b = (struct sockaddr_in *) &(ifreqs[i].ifr_addr);

			inet_ntop(AF_INET, &(b->sin_addr), ip_addr, INET_ADDRSTRLEN);
			return string(ip_addr);
		}
	}
	return NULL;
}

///Returns usage tips
string Nova::LocalTrafficMonitor::Usage()
{
	string usage_tips = "Local Traffic Monitor Module\n";
	usage_tips += "\tUsage:Local Traffic Monitor Module -l <log config file> -n <NOVA config file> \n";
	usage_tips += "\t-l: Path to LOG4CXX config xml file.\n";
	usage_tips += "\t-n: Path to NOVA config txt file. (Config/NOVAConfig_LTM.txt by default)\n";
	return usage_tips;
}

void Nova::LocalTrafficMonitor::LoadConfig(char* input)
{
	//Used to verify all values have been loaded
	bool verify[3];
	for(uint i = 0; i < 3; i++)
		verify[i] = false;

	string line;
	string prefix;
	ifstream config(input);

	if(config.is_open())
	{
		while(config.good())
		{
			getline(config,line);
			prefix = "INTERFACE";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(line.size() > 0)
				{
					dev = line;
					verify[0]=true;
				}
				continue;

			}
			prefix = "TCP_TIMEOUT";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					tcpTime = atoi(line.c_str());
					verify[1]=true;
				}
				continue;
			}
			prefix = "TCP_CHECK_FREQ";
			if(!line.substr(0,prefix.size()).compare(prefix))
			{
				line = line.substr(prefix.size()+1,line.size());
				if(atoi(line.c_str()) > 0)
				{
					tcpFreq = atoi(line.c_str());
					verify[2]=true;
				}
				continue;

			}
			prefix = "#";
			if(line.substr(0,prefix.size()).compare(prefix) && line.compare(""))
			{
				LOG4CXX_INFO(m_logger,"Unexpected entry in NOVA configuration file" << errno);
				continue;
			}
		}

		//Checks to make sure all values have been set.
		bool v = true;
		for(uint i = 0; i < 3; i++)
		{
			v &= verify[i];
		}

		if(v == false)
		{
			LOG4CXX_ERROR(m_logger,"One or more values have not been set" << errno);
			exit(1);
		}
		else
		{
			LOG4CXX_INFO(m_logger,"Config loaded successfully" << errno);
		}
	}
	else
	{
		LOG4CXX_INFO(m_logger, "No configuration file detected." << errno );
		exit(1);
	}
	config.close();
}
