//============================================================================
// Name        : Evidence.cpp
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
// Description : Evidence object represents a preprocessed ip packet
//					for inclusion in a Suspect's Feature Set
//============================================================================/*

#include "Evidence.h"
#include "netinet/tcp.h"
#include <dumbnet.h>

using namespace std;

namespace Nova
{

//Default Constructor
Evidence::Evidence()
{
	m_next = NULL;
	m_evidencePacket.dst_port = 0;
	m_evidencePacket.ip_dst = 0;
	m_evidencePacket.ip_len = 0;
	m_evidencePacket.ip_p = 0;
	m_evidencePacket.ip_src = ~0;
	m_evidencePacket.ts = 0;
}

Evidence::Evidence(const u_char *packet_at_ip_header, const pcap_pkthdr *pkthdr)
{

	struct ip_hdr *ip;
	ip = (ip_hdr *) packet_at_ip_header;

	//Get timestamp
	m_evidencePacket.ts = pkthdr->ts.tv_sec;

	// 00001111 mask to get the ip header length
	uint8_t ip_hl = ip->ip_hl;//15;
	m_next = NULL;

	//offset += 2;
	m_evidencePacket.ip_len = ntohs(ip->ip_len);//ntohs(*(uint16_t *)offset);
	//offset += 7; // @16 - read 1
	m_evidencePacket.ip_p = ip->ip_p;//*(uint8_t *)offset;
	//offset += 3; // @19 - read 4
	m_evidencePacket.ip_src = ntohl(ip->ip_src); //ntohl(*(uint32_t *)offset);
	//offset += 4; // @23 - read 4
	m_evidencePacket.ip_dst = ntohl(ip->ip_dst);//ntohl(*(uint32_t *)offset);

	//Initialize port to 0 in case we don't have a TCP or UDP packet
	m_evidencePacket.dst_port = -1;

	if((m_evidencePacket.ip_p == 6))
	{
		struct tcp_hdr *tcp;
		tcp = (tcp_hdr *) (packet_at_ip_header + ip_hl*4);
		//read in the dest port
		m_evidencePacket.dst_port = ntohs(tcp->th_dport);
	}
	else if(m_evidencePacket.ip_p == 17)//udp
	{
		struct udp_hdr *udp;
		udp = (udp_hdr *) (packet_at_ip_header + ip_hl*4);
		m_evidencePacket.dst_port = ntohs(udp->uh_dport);
	}

	if(m_evidencePacket.ip_p == 6)
	{
		struct tcphdr *tcp;
		tcp = (tcphdr*)(packet_at_ip_header + ip_hl*4);
		m_evidencePacket.tcp_hdr.ack = tcp->ack;
		m_evidencePacket.tcp_hdr.rst = tcp->rst;
		m_evidencePacket.tcp_hdr.syn = tcp->syn;
		m_evidencePacket.tcp_hdr.fin = tcp->fin;
	}
}

Evidence::Evidence(Evidence *evidence)
{
	m_evidencePacket = evidence->m_evidencePacket;
	m_next = NULL;
}

}
