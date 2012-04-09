//============================================================================
// Name        : HoneydConfiguration.cpp
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
// Description : Object for reading and writing Honeyd XML configurations
//============================================================================/*

#include "HoneydConfiguration.h"
#include "NovaUtil.h"
#include "Logger.h"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <math.h>

using namespace std;
using namespace Nova;
using boost::property_tree::ptree;
using boost::property_tree::xml_parser::trim_whitespace;


HoneydConfiguration::HoneydConfiguration()
{
	//TODO Implement once we support multiple configurations
	/*switch(Config::Inst()->GetHaystackStorage())
	{
		default:
		{
			break;
		}
	}*/
	m_homePath = Config::Inst()->GetPathHome();
	m_subnets.set_empty_key("");
	m_ports.set_empty_key("");
	m_nodes.set_empty_key("");
	m_profiles.set_empty_key("");
	m_scripts.set_empty_key("");
	m_subnets.set_deleted_key("Deleted");
	m_nodes.set_deleted_key("Deleted");
	m_profiles.set_deleted_key("Deleted");
	m_ports.set_deleted_key("Deleted");
	m_scripts.set_deleted_key("Deleted");
}

int HoneydConfiguration::GetMaskBits(in_addr_t mask)
{
	mask = ~mask;
	int i = 32;
	while(mask != 0)
	{
		mask = mask/2;
		i--;
	}
	return i;
}

//Calls all load functions
void HoneydConfiguration::LoadAllTemplates()
{
	m_scripts.clear_no_resize();
	m_ports.clear_no_resize();
	m_profiles.clear_no_resize();
	m_nodes.clear_no_resize();
	m_subnets.clear_no_resize();

	LoadScriptsTemplate();
	LoadPortsTemplate();
	LoadProfilesTemplate();
	LoadNodesTemplate();
}

//Loads ports from file
void HoneydConfiguration::LoadPortsTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;

	m_portTree.clear();
	try
	{
		read_xml(m_homePath+"/templates/ports.xml", m_portTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &v, m_portTree.get_child("ports"))
		{
			port p;
			p.tree = v.second;
			//Required xml entries
			p.portName = v.second.get<std::string>("name");

			if(!p.portName.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.", "");
				continue;
			}

			p.portNum = v.second.get<std::string>("number");
			p.type = v.second.get<std::string>("type");
			p.behavior = v.second.get<std::string>("behavior");

			//If this port uses a script, find and assign it.
			if(!p.behavior.compare("script") || !p.behavior.compare("internal"))
			{
				p.scriptName = v.second.get<std::string>("script");
			}
			//If the port works as a proxy, find destination
			else if(!p.behavior.compare("proxy"))
			{
				p.proxyIP = v.second.get<std::string>("IP");
				p.proxyPort = v.second.get<std::string>("Port");
			}
			m_ports[p.portName] = p;
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading ports: "+string(e.what())+".", "");
	}
}


//Loads the subnets and nodes from file for the currently specified group
void HoneydConfiguration::LoadNodesTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;

	m_groupTree.clear();
	ptree ptr;

	try
	{
		read_xml(m_homePath+"/templates/nodes.xml", m_groupTree, boost::property_tree::xml_parser::trim_whitespace);
		BOOST_FOREACH(ptree::value_type &v, m_groupTree.get_child("groups"))
		{
			//Find the specified group
			if(!v.second.get<std::string>("name").compare(Config::Inst()->GetGroup()))
			{
				try //Null Check
				{
					//Load Subnets first, they are needed before we can load nodes
					m_subnetTree = v.second.get_child("subnets");
					LoadSubnets(&m_subnetTree);

					try //Null Check
					{
						//If subnets are loaded successfully, load nodes
						m_nodesTree = v.second.get_child("nodes");
						LoadNodes(&m_nodesTree);
					}
					catch(std::exception &e)
					{
						LOG(ERROR, "Problem loading nodes: "+string(e.what())+".", "");
					}
				}
				catch(std::exception &e)
				{
					LOG(ERROR, "Problem loading subnets: "+string(e.what())+".", "");
				}
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading groups: "+Config::Inst()->GetGroup()+" - "+string(e.what()) +".", "");
	}
}


//Sets the configuration of 'set' values for profile that called it
void HoneydConfiguration::LoadProfileSettings(ptree *ptr, profile *p)
{
	string prefix;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			prefix = "TCP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->tcpAction = v.second.data();
				p->inherited[TCP_ACTION] = false;
				continue;
			}
			prefix = "UDP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->udpAction = v.second.data();
				p->inherited[UDP_ACTION] = false;
				continue;
			}
			prefix = "ICMP";
			if(!string(v.first.data()).compare(prefix))
			{
				p->icmpAction = v.second.data();
				p->inherited[ICMP_ACTION] = false;
				continue;
			}
			prefix = "personality";
			if(!string(v.first.data()).compare(prefix))
			{
				p->personality = v.second.data();
				p->inherited[PERSONALITY] = false;
				continue;
			}
			prefix = "ethernet";
			if(!string(v.first.data()).compare(prefix))
			{
				p->ethernet = v.second.data();
				p->inherited[ETHERNET] = false;
				continue;
			}
			prefix = "uptimeMin";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptimeMin = v.second.data();
				p->inherited[UPTIME] = false;
				continue;
			}
			prefix = "uptimeMax";
			if(!string(v.first.data()).compare(prefix))
			{
				p->uptimeMax = v.second.data();
				p->inherited[UPTIME] = false;
				continue;
			}
			prefix = "dropRate";
			if(!string(v.first.data()).compare(prefix))
			{
				p->dropRate = v.second.data();
				p->inherited[DROP_RATE] = false;
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading profile set parameters: "+string(e.what())+".", "");
	}
}

//Adds specified ports and subsystems
// removes any previous port with same number and type to avoid conflicts
void HoneydConfiguration::LoadProfileServices(ptree *ptr, profile *p)
{
	string prefix;
	port * prt;

	try
	{
		for(uint i = 0; i < p->ports.size(); i++)
		{
			p->ports[i].second = true;
		}
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			//Checks for ports
			prefix = "ports";
			if(!string(v.first.data()).compare(prefix))
			{
				//Iterates through the ports
				BOOST_FOREACH(ptree::value_type &v2, ptr->get_child("ports"))
				{
					prt = &m_ports[v2.second.data()];

					//Checks inherited ports for conflicts
					for(uint i = 0; i < p->ports.size(); i++)
					{
						//Erase inherited port if a conflict is found
						if(!prt->portNum.compare(m_ports[p->ports[i].first].portNum) && !prt->type.compare(m_ports[p->ports[i].first].type))
						{
							p->ports.erase(p->ports.begin()+i);
						}
					}
					//Add specified port
					pair<string, bool> portPair;
					portPair.first = prt->portName;
					portPair.second = false;
					if(!p->ports.size())
					{
						p->ports.push_back(portPair);
					}
					else
					{
						uint i = 0;
						for(i = 0; i < p->ports.size(); i++)
						{
							port * temp = &m_ports[p->ports[i].first];
							if((atoi(temp->portNum.c_str())) < (atoi(prt->portNum.c_str())))
							{
								continue;
							}
							break;
						}
						if(i < p->ports.size())
						{
							p->ports.insert(p->ports.begin()+i, portPair);
						}
						else
						{
							p->ports.push_back(portPair);
						}
					}
				}
				continue;
			}

			//Checks for a subsystem
			prefix = "subsystem"; //TODO
			if(!string(v.first.data()).compare(prefix))
			{
				continue;
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading profile add parameters: "+string(e.what())+".", "");
	}
}

//Recursive descent down a profile tree, inherits parent, sets values and continues if not leaf.
void HoneydConfiguration::LoadProfileChildren(string parent)
{
	ptree ptr = m_profiles[parent].tree;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr.get_child("profiles"))
		{
			ptree *ptr2;

			//Inherits parent,
			profile prof = m_profiles[parent];
			prof.tree = v.second;
			prof.parentProfile = parent;

			//Gets name, initializes DHCP
			prof.name = v.second.get<std::string>("name");

			if(!prof.name.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.", "");
				continue;
			}

			for(uint i = 0; i < INHERITED_MAX; i++)
			{
				prof.inherited[i] = true;
			}

			try //Conditional: If profile overrides type
			{
				prof.type = (profileType)v.second.get<int>("type");
				prof.inherited[TYPE] = false;
			}
			catch(...){}

			try //Conditional: If profile has set configurations different from parent
			{
				ptr2 = &v.second.get_child("set");
				LoadProfileSettings(ptr2, &prof);
			}
			catch(...){}

			try //Conditional: If profile has port or subsystems different from parent
			{
				ptr2 = &v.second.get_child("add");
				LoadProfileServices(ptr2, &prof);
			}
			catch(...){}

			//Saves the profile
			m_profiles[prof.name] = prof;

			try //Conditional: if profile has children (not leaf)
			{
				LoadProfileChildren(prof.name);
			}
			catch(...){}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading sub profiles: "+string(e.what())+".", "");
	}
}


//Loads scripts from file
void HoneydConfiguration::LoadScriptsTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	m_scriptTree.clear();
	try
	{
		read_xml(m_homePath+"/scripts.xml", m_scriptTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &v, m_scriptTree.get_child("scripts"))
		{
			script s;
			s.tree = v.second;
			//Each script consists of a name and path to that script
			s.name = v.second.get<std::string>("name");

			if(!s.name.compare(""))
			{
				LOG(ERROR, "Problem loading honeyd XML files.","");
				continue;
			}

			s.path = v.second.get<std::string>("path");
			m_scripts[s.name] = s;
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading scripts: "+string(e.what())+".", "");
	}
}


/************************************************
 * Save Honeyd XML Configuration Functions
 ************************************************/

//Saves the current configuration information to XML files

//**Important** this function assumes that unless it is a new item (ptree pointer == NULL) then
// all required fields exist and old fields have been removed. Ex: if a port previously used a script
// but now has a behavior of open, at that point the user should have erased the script field.
// inverserly if a user switches to script the script field must be created.

//To summarize this function only populates the xml data for the values it contains unless it is a new item,
// it does not clean up, and only creates if it's a new item and then only for the fields that are needed.
// it does not track profile inheritance either, that should be created when the heirarchy is modified.
void HoneydConfiguration::SaveAllTemplates()
{
	using boost::property_tree::ptree;
	ptree pt;

	//Scripts
	m_scriptTree.clear();
	for(ScriptTable::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		pt = it->second.tree;
		pt.put<std::string>("name", it->second.name);
		pt.put<std::string>("path", it->second.path);
		m_scriptTree.add_child("scripts.script", pt);
	}

	//Ports
	m_portTree.clear();
	for(PortTable::iterator it = m_ports.begin(); it != m_ports.end(); it++)
	{
		pt = it->second.tree;
		pt.put<std::string>("name", it->second.portName);
		pt.put<std::string>("number", it->second.portNum);
		pt.put<std::string>("type", it->second.type);
		pt.put<std::string>("behavior", it->second.behavior);
		//If this port uses a script, save it.
		if(!it->second.behavior.compare("script") || !it->second.behavior.compare("internal"))
		{
			pt.put<std::string>("script", it->second.scriptName);
		}
		//If the port works as a proxy, save destination
		else if(!it->second.behavior.compare("proxy"))
		{
			pt.put<std::string>("IP", it->second.proxyIP);
			pt.put<std::string>("Port", it->second.proxyPort);
		}
		m_portTree.add_child("ports.port", pt);
	}

	m_subnetTree.clear();
	for(SubnetTable::iterator it = m_subnets.begin(); it != m_subnets.end(); it++)
	{
		pt = it->second.tree;

		//TODO assumes subnet is interface, need to discover and handle if virtual
		pt.put<std::string>("name", it->second.name);
		pt.put<bool>("enabled",it->second.enabled);
		pt.put<bool>("isReal", it->second.isRealDevice);

		//Remove /## format mask from the address then put it in the XML.
		stringstream ss;
		ss << "/" << it->second.maskBits;
		int i = ss.str().size();
		string temp = it->second.address.substr(0,(it->second.address.size()-i));
		pt.put<std::string>("IP", temp);

		//Gets the mask from mask bits then put it in XML
		in_addr_t mask = ::pow(2, 32-it->second.maskBits) - 1;
		//If maskBits is 24 then we have 2^8 -1 = 0x000000FF
		mask = ~mask; //After getting the inverse of this we have the mask in host addr form.
		//Convert to network order, put in in_addr struct
		//call ntoa to get char * and make string
		in_addr tempMask;
		tempMask.s_addr = htonl(mask);
		temp = string(inet_ntoa(tempMask));
		pt.put<std::string>("mask", temp);
		m_subnetTree.add_child("interface", pt);
	}

	//Nodes
	m_nodesTree.clear();
	for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		pt = it->second.tree;
		//Required xml entires
		pt.put<std::string>("interface", it->second.interface);
		pt.put<std::string>("IP", it->second.IP);
		pt.put<bool>("enabled", it->second.enabled);
		pt.put<std::string>("name", it->second.name);
		if(it->second.MAC.size())
			pt.put<std::string>("MAC", it->second.MAC);
		pt.put<std::string>("profile.name", it->second.pfile);
		m_nodesTree.add_child("node",pt);
	}
	using boost::property_tree::ptree;
	BOOST_FOREACH(ptree::value_type &v, m_groupTree.get_child("groups"))
	{
		//Find the specified group
		if(!v.second.get<std::string>("name").compare(Config::Inst()->GetGroup()))
		{
			//Load Subnets first, they are needed before we can load nodes
			v.second.put_child("subnets", m_subnetTree);
			v.second.put_child("nodes",m_nodesTree);
		}
	}
	m_profileTree.clear();
	for(ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(it->second.parentProfile == "")
		{
			pt = it->second.tree;
			m_profileTree.add_child("profiles.profile", pt);
		}
	}
	boost::property_tree::xml_writer_settings<char> settings('\t', 1);
	write_xml(m_homePath+"/scripts.xml", m_scriptTree, std::locale(), settings);
	if(system("cd templates/") == -1)
	{
		if(system("mkdir templates") == -1)
		{
			//TODO Log error here
		}
	}
	write_xml(m_homePath+"/templates/ports.xml", m_portTree, std::locale(), settings);
	write_xml(m_homePath+"/templates/nodes.xml", m_groupTree, std::locale(), settings);
	write_xml(m_homePath+"/templates/profiles.xml", m_profileTree, std::locale(), settings);
}

//Writes the current configuration to honeyd configs
void HoneydConfiguration::WriteHoneydConfiguration(string path)
{
	stringstream out;

	vector<string> profilesParsed;

	for (ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if(!it->second.parentProfile.compare(""))
		{
			string pString = ProfileToString(&it->second);
			out << pString;
			profilesParsed.push_back(it->first);
		}
	}

	while (profilesParsed.size() < m_profiles.size())
	{
		for (ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
		{
			bool selfMatched = false;
			bool parentFound = false;
			for (uint i = 0; i < profilesParsed.size(); i++)
			{
				if(!it->second.parentProfile.compare(profilesParsed[i]))
				{
					parentFound = true;
					continue;
				}
				if(!it->first.compare(profilesParsed[i]))
				{
					selfMatched = true;
					break;
				}
			}

			if(!selfMatched && parentFound)
			{
				string pString = ProfileToString(&it->second);
				out << pString;
				profilesParsed.push_back(it->first);

			}
		}
	}

	// Start node section
	for (NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if(!it->second.enabled)
		{
			continue;
		}
		//We write the dopp regardless of whether or not it is enabled so that it can be toggled during runtime.
		else if(!it->second.name.compare("Doppelganger"))
		{
			string pString = DoppProfileToString(&m_profiles[it->second.pfile]);
			out << endl << pString;
			out << "bind " << it->second.IP << " DoppelgangerReservedTemplate" << endl << endl;
			//Use configured or discovered loopback
		}
		else switch (m_profiles[it->second.pfile].type)
		{
			case static_IP:
			{
				out << "bind " << it->second.IP << " " << it->second.pfile << endl;
				if(it->second.MAC.compare(""))
				{
					out << "set " << it->second.IP << " ethernet \"" << it->second.MAC << "\"" << endl;
				}
				break;
			}
			case staticDHCP:
			{
				out << "dhcp " << it->second.pfile << " on " << it->second.interface << " ethernet \"" << it->second.MAC << "\"" << endl;
				break;
			}
			case randomDHCP:
			{
				out << "dhcp " << it->second.pfile << " on " << it->second.interface << endl;
				break;
			}
			default:
			{
				break;
			}
		}
	}
	ofstream outFile(path);
	outFile << out.str() << endl;
	outFile.close();
}


//loads subnets from file for current group
void HoneydConfiguration::LoadSubnets(ptree *ptr)
{
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			//If real interface
			if(!string(v.first.data()).compare("interface"))
			{
				subnet sub;
				sub.tree = v.second;
				sub.isRealDevice =  v.second.get<bool>("isReal");
				//Extract the data
				sub.name = v.second.get<std::string>("name");
				sub.address = v.second.get<std::string>("IP");
				sub.mask = v.second.get<std::string>("mask");
				sub.enabled = v.second.get<bool>("enabled");

				//Gets the IP address in uint32 form
				in_addr_t baseTemp = ntohl(inet_addr(sub.address.c_str()));

				//Converting the mask to uint32 allows a simple bitwise AND to get the lowest IP in the subnet.
				in_addr_t maskTemp = ntohl(inet_addr(sub.mask.c_str()));
				sub.base = (baseTemp & maskTemp);
				//Get the number of bits in the mask
				sub.maskBits = GetMaskBits(maskTemp);
				//Adding the binary inversion of the mask gets the highest usable IP
				sub.max = sub.base + ~maskTemp;
				stringstream ss;
				ss << sub.address << "/" << sub.maskBits;
				sub.address = ss.str();

				//Save subnet
				m_subnets[sub.name] = sub;
			}
			//If virtual honeyd subnet
			else if(!string(v.first.data()).compare("virtual"))
			{
				//TODO Implement and test
				/*subnet sub;
				sub.tree = v.second;
				sub.isRealDevice = false;
				//Extract the data
				sub.name = v.second.get<std::string>("name");
				sub.address = v.second.get<std::string>("IP");
				sub.mask = v.second.get<std::string>("mask");
				sub.enabled = v.second.get<bool>("enabled");

				//Gets the IP address in uint32 form
				in_addr_t baseTemp = ntohl(inet_addr(sub.address.c_str()));

				//Converting the mask to uint32 allows a simple bitwise AND to get the lowest IP in the subnet.
				in_addr_t maskTemp = ntohl(inet_addr(sub.mask.c_str()));
				sub.base = (baseTemp & maskTemp);
				//Get the number of bits in the mask
				sub.maskBits = GetMaskBits(maskTemp);
				//Adding the binary inversion of the mask gets the highest usable IP
				sub.max = sub.base + ~maskTemp;
				stringstream ss;
				ss << sub.address << "/" << sub.maskBits;
				sub.address = ss.str();

				//Save subnet
				subnets[sub.name] = sub;*/
			}
			else
			{
				LOG(ERROR, "Unexpected Entry in file: "+string(v.first.data())+".", "");
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading subnets: "+string(e.what()), "");
	}
}


//loads haystack nodes from file for current group
void HoneydConfiguration::LoadNodes(ptree *ptr)
{
	profile p;
	//ptree * ptr2;
	try
	{
		BOOST_FOREACH(ptree::value_type &v, ptr->get_child(""))
		{
			if(!string(v.first.data()).compare("node"))
			{
				node n;
				int max = 0;
				bool unique = true;
				stringstream ss;
				uint i = 0, j = 0;
				j = ~j; // 2^32-1

				n.tree = v.second;
				//Required xml entires
				n.interface = v.second.get<std::string>("interface");
				n.IP = v.second.get<std::string>("IP");
				n.enabled = v.second.get<bool>("enabled");
				n.pfile = v.second.get<std::string>("profile.name");

				if(!n.pfile.compare(""))
				{
					LOG(ERROR, "Problem loading honeyd XML files.", "");
					continue;
				}

				p = m_profiles[n.pfile];

				//Get mac if present
				try //Conditional: has "set" values
				{
					//ptr2 = &v.second.get_child("MAC");
					//pass 'set' subset and pointer to this profile
					n.MAC = v.second.get<std::string>("MAC");
				}
				catch(...){}
				if(!n.IP.compare(Config::Inst()->GetDoppelIp()))
				{
					n.name = "Doppelganger";
					n.sub = n.interface;
					n.realIP = htonl(inet_addr(n.IP.c_str())); //convert ip to uint32
					//save the node in the table
					m_nodes[n.name] = n;

					//Put address of saved node in subnet's list of nodes.
					m_subnets[m_nodes[n.name].sub].nodes.push_back(n.name);
				}
				else switch(p.type)
				{

					//***** STATIC IP ********//
					case static_IP:
					{

						n.name = n.IP;

						if(!n.name.compare(""))
						{
							LOG(ERROR, "Problem loading honeyd XML files.", "");
							continue;
						}

						//intialize subnet to NULL and check for smallest bounding subnet
						n.sub = ""; //TODO virtual subnets will need to be handled when implemented
						n.realIP = htonl(inet_addr(n.IP.c_str())); //convert ip to uint32
						//Tracks the mask with smallest range by comparing num of bits used.

						//Check each subnet
						for(SubnetTable::iterator it = m_subnets.begin(); it != m_subnets.end(); it++)
						{
							//If node falls outside a subnets range skip it
							if((n.realIP < it->second.base) || (n.realIP > it->second.max))
							{
								continue;
							}
							//If this is the smallest range
							if(it->second.maskBits > max)
							{
								//If node isn't using host's address
								if(it->second.address.compare(n.IP))
								{
									max = it->second.maskBits;
									n.sub = it->second.name;
								}
							}
						}

						//Check that node has unique IP addr
						for(NodeTable::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
						{
							if(n.realIP == it->second.realIP)
							{
								unique = false;
							}
						}

						//If we have a subnet and node is unique
						if((n.sub != "") && unique)
						{
							//save the node in the table
							m_nodes[n.name] = n;

							//Put address of saved node in subnet's list of nodes.
							m_subnets[m_nodes[n.name].sub].nodes.push_back(n.name);
						}
						//If no subnet found, can't use node unless it's doppelganger.
						else
						{
							LOG(ERROR, "Node at IP: "+ n.IP+"is outside all valid subnet ranges.", "");
						}
						break;
					}

					//***** STATIC DHCP (static MAC) ********//
					case staticDHCP:
					{
						//If no MAC is set, there's a problem
						if(!n.MAC.size())
						{
							LOG(ERROR, "DHCP Enabled node using profile: "+ n.pfile+"does not have a MAC Address.", "");
							continue;
						}

						//Associated MAC is already in use, this is not allowed, throw out the node
						if(m_nodes.find(n.MAC) != m_nodes.end())
						{
							LOG(ERROR, "Duplicate MAC address detected in node: "+ n.MAC, "");
							continue;
						}
						n.name = n.MAC;

						if(!n.name.compare(""))
						{
							LOG(ERROR, "Problem loading honeyd XML files.", "");
							continue;
						}

						n.sub = n.interface; //TODO virtual subnets will need to be handled when implemented
						// If no valid subnet/interface found
						if(!n.sub.compare(""))
						{
							LOG(ERROR, "DHCP Enabled Node with MAC: "+n.MAC+" is unable to resolve it's interface.","");
							continue;
						}

						//save the node in the table
						m_nodes[n.name] = n;

						//Put address of saved node in subnet's list of nodes.
						m_subnets[m_nodes[n.name].sub].nodes.push_back(n.name);
						break;
					}

					//***** RANDOM DHCP (random MAC each time run) ********//
					case randomDHCP:
					{
						n.name = n.pfile + " on " + n.interface;

						if(!n.name.compare(""))
						{
							LOG(ERROR, "Problem loading honeyd XML files.", "");
							continue;
						}

						//Finds a unique identifier
						while((m_nodes.find(n.name) != m_nodes.end()) && (i < j))
						{
							i++;
							ss.str("");
							ss << n.pfile << " on " << n.interface << "-" << i;
							n.name = ss.str();
						}
						n.sub = n.interface; //TODO virtual subnets will need to be handled when implemented
						// If no valid subnet/interface found
						if(!n.sub.compare(""))
						{
							LOG(ERROR, "DHCP Enabled Node is unable to resolve it's interface: " +n.interface,"");
							continue;
						}
						//save the node in the table
						m_nodes[n.name] = n;

						//Put address of saved node in subnet's list of nodes.
						m_subnets[m_nodes[n.name].sub].nodes.push_back(n.name);
						break;
					}
					default:
					{
						break;
					}
				}
			}
			else
			{
				LOG(ERROR, "Unexpected Entry in file: "+string(v.first.data()), "");
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading nodes: "+ string(e.what()), "");
	}
}

void HoneydConfiguration::LoadProfilesTemplate()
{
	using boost::property_tree::ptree;
	using boost::property_tree::xml_parser::trim_whitespace;
	ptree * ptr;
	m_profileTree.clear();
	try
	{
		read_xml(m_homePath+"/templates/profiles.xml", m_profileTree, boost::property_tree::xml_parser::trim_whitespace);

		BOOST_FOREACH(ptree::value_type &v, m_profileTree.get_child("profiles"))
		{
			//Generic profile, essentially a honeyd template
			if(!string(v.first.data()).compare("profile"))
			{
				profile p;
				//Root profile has no parent
				p.parentProfile = "";
				p.tree = v.second;

				//Name required, DCHP boolean intialized (set in loadProfileSet)
				p.name = v.second.get<std::string>("name");

				if(!p.name.compare(""))
				{
					LOG(ERROR, "Problem loading honeyd XML files.", "");
					continue;
				}

				p.ports.clear();
				p.type = (profileType)v.second.get<int>("type");
				for(uint i = 0; i < INHERITED_MAX; i++)
				{
					p.inherited[i] = false;
				}

				try //Conditional: has "set" values
				{
					ptr = &v.second.get_child("set");
					//pass 'set' subset and pointer to this profile
					LoadProfileSettings(ptr, &p);
				}
				catch(...){}

				try //Conditional: has "add" values
				{
					ptr = &v.second.get_child("add");
					//pass 'add' subset and pointer to this profile
					LoadProfileServices(ptr, &p);
				}
				catch(...){}

				//Save the profile
				m_profiles[p.name] = p;

				try //Conditional: has children profiles
				{
					//start recurisive descent down profile tree with this profile as the root
					//pass subtree and pointer to parent
					LoadProfileChildren(p.name);
				}
				catch(...){}

			}

			//Honeyd's implementation of switching templates based on conditions
			else if(!string(v.first.data()).compare("dynamic"))
			{
				//TODO
			}
			else
			{
				LOG(ERROR, "Invalid XML Path " +string(v.first.data())+".", "");
			}
		}
	}
	catch(std::exception &e)
	{
		LOG(ERROR, "Problem loading Profiles: "+string(e.what())+".", "");
	}
}

string HoneydConfiguration::ProfileToString(profile* p)
{
	stringstream out;

	if(!p->parentProfile.compare("default") || !p->parentProfile.compare(""))
	{
		out << "create " << p->name << endl;
	}
	else
	{
		out << "clone " << p->parentProfile << " " << p->name << endl;
	}

	out << "set " << p->name  << " default tcp action " << p->tcpAction << endl;
	out << "set " << p->name  << " default udp action " << p->udpAction << endl;
	out << "set " << p->name  << " default icmp action " << p->icmpAction << endl;

	if(p->personality.compare(""))
	{
		out << "set " << p->name << " personality \"" << p->personality << '"' << endl;
	}

	if(p->ethernet.compare(""))
	{
		out << "set " << p->name << " ethernet \"" << p->ethernet << '"' << endl;
	}


	if(p->dropRate.compare(""))
	{
		out << "set " << p->name << " droprate in " << p->dropRate << endl;
	}

	for (uint i = 0; i < p->ports.size(); i++)
	{
		// Only include non-inherited ports
		if(!p->ports[i].second)
		{
			out << "add " << p->name;
			if(!m_ports[p->ports[i].first].type.compare("TCP"))
			{
				out << " tcp port ";
			}
			else
			{
				out << " udp port ";
			}
			out << m_ports[p->ports[i].first].portNum << " ";

			if(!(m_ports[p->ports[i].first].behavior.compare("script")))
			{
				string scriptName = m_ports[p->ports[i].first].scriptName;

				if(m_scripts[scriptName].path.compare(""))
				{
					out << '"' << m_scripts[scriptName].path << '"'<< endl;
				}
				else
				{
					LOG(ERROR, "Error writing profile port script.", "Path to script "+scriptName+" is null.");
				}
			}
			else
			{
				out << m_ports[p->ports[i].first].behavior << endl;
			}
		}
	}
	out << endl;
	return out.str();
}

//
string HoneydConfiguration::DoppProfileToString(profile* p)
{
	stringstream out;
	out << "create DoppelgangerReservedTemplate" << endl;

	out << "set DoppelgangerReservedTemplate default tcp action " << p->tcpAction << endl;
	out << "set DoppelgangerReservedTemplate default udp action " << p->udpAction << endl;
	out << "set DoppelgangerReservedTemplate default icmp action " << p->icmpAction << endl;

	if(p->personality.compare(""))
	{
		out << "set DoppelgangerReservedTemplate" << " personality \"" << p->personality << '"' << endl;
	}


	if(p->dropRate.compare(""))
	{
		out << "set DoppelgangerReservedTemplate" << " droprate in " << p->dropRate << endl;
	}

	for (uint i = 0; i < p->ports.size(); i++)
	{
		// Only include non-inherited ports
		if(!p->ports[i].second)
		{
			out << "add DoppelgangerReservedTemplate";
			if(!m_ports[p->ports[i].first].type.compare("TCP"))
			{
				out << " tcp port ";
			}
			else
			{
				out << " udp port ";
			}
			out << m_ports[p->ports[i].first].portNum << " ";

			if(!(m_ports[p->ports[i].first].behavior.compare("script")))
			{
				string scriptName = m_ports[p->ports[i].first].scriptName;

				if(m_scripts[scriptName].path.compare(""))
				{
					out << '"' << m_scripts[scriptName].path << '"'<< endl;
				}
				else
				{
					LOG(ERROR, "Error writing profile port script.", "Path to script "+scriptName+" is null.");
				}
			}
			else
			{
				out << m_ports[p->ports[i].first].behavior << endl;
			}
		}
	}
	out << endl;
	return out.str();
}

SubnetTable HoneydConfiguration::GetSubnets() const
{
	return m_subnets;
}
PortTable HoneydConfiguration::GetPorts() const
{
	return m_ports;
}
NodeTable HoneydConfiguration::GetNodes() const
{
	return m_nodes;
}
ScriptTable HoneydConfiguration::GetScripts() const
{
	return m_scripts;
}
ProfileTable HoneydConfiguration::GetProfiles() const
{
	return m_profiles;
}

void HoneydConfiguration::SetSubnets(SubnetTable subnets)
{
	m_subnets.clear_no_resize();
	m_subnets = subnets;
}
void HoneydConfiguration::SetPorts(PortTable ports)
{
	m_ports.clear_no_resize();
	m_ports = ports;
}
void HoneydConfiguration::SetNodes(NodeTable nodes)
{
	m_nodes.clear_no_resize();
	m_nodes = nodes;
}
void HoneydConfiguration::SetScripts(ScriptTable scripts)
{
	m_scripts.clear_no_resize();
	m_scripts = scripts;
}
void HoneydConfiguration::SetProfiles(ProfileTable profiles)
{
	m_profiles.clear_no_resize();
	m_profiles = profiles;
}

//Setter for the directory to read from and write to
void HoneydConfiguration::SetHomePath(std::string homePath)
{
	m_homePath = homePath;
}

//Getter for the directory to read from and write to
std::string HoneydConfiguration::GetHomePath()
{
	return m_homePath;
}



// TODO TODO TODO
std::vector<std::string> HoneydConfiguration::GetProfileChildren(std::string parent)
{
	vector<std::string> childProfiles;

	for (ProfileTable::iterator it = m_profiles.begin(); it != m_profiles.end(); it++)
	{
		if (it->second.parentProfile == parent)
		{
			childProfiles.push_back(it->second.name);
		}
	}

	return childProfiles;
}

std::pair<hdConfigReturn, profileType> HoneydConfiguration::GetProfileType(profileName profile)
{
	pair<hdConfigReturn, profileType> ret;

	ProfileTable::iterator it = m_profiles.find(profile);
	if (it == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = profileType::static_IP;
		return ret;
	}

	ret.first = NOT_INHERITED;
	ret.second = it->second.type;
	return ret;
}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetEthernet(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if (m_profiles.find(profile) == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].ethernet == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].ethernet;

	return ret;
}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetPersonality(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if (m_profiles.find(profile) == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].personality == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].personality;

	return ret;
}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetDroprate(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if (m_profiles.find(profile) == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].dropRate == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].dropRate;

	return ret;

}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetActionTCP(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if (m_profiles.find(profile) == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].tcpAction == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].tcpAction;

	return ret;

}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetActionUDP(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if (m_profiles.find(profile) == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].udpAction == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].udpAction;

	return ret;
}

std::pair<hdConfigReturn, std::string> HoneydConfiguration::GetActionICMP(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if (m_profiles.find(profile) == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].icmpAction == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].icmpAction;

	return ret;

}

std::pair<hdConfigReturn, string> HoneydConfiguration::GetUptimeMin(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if (m_profiles.find(profile) == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].uptimeMin == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].uptimeMin;

	return ret;
}

std::pair<hdConfigReturn, string> HoneydConfiguration::GetUptimeMax(profileName profile)
{
	pair<hdConfigReturn, string> ret;

	// Make sure the input profile name exists
	if (m_profiles.find(profile) == m_profiles.end())
	{
		ret.first = NO_SUCH_KEY;
		ret.second = "";
		return ret;
	}

	ret.first = NOT_INHERITED;
	profileName parent = profile;

	while (m_profiles[parent].uptimeMax == "")
	{
		ret.first = INHERITED;
		parent = m_profiles[parent].parentProfile;
	}

	ret.second = m_profiles[parent].uptimeMax;

	return ret;
}
