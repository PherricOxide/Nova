//============================================================================
// Name        : nodePopup.cpp
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
// Description : Popup for creating and editing nodes
//============================================================================
#include "nodePopup.h"
#include "novaconfig.h"
#include "novagui.h"

using namespace std;
using namespace Nova;

//Parent window pointers, allows us to call functions from the parent
NovaConfig * novaParent;

/************************************************
 * Construct and Initialize GUI
 ************************************************/

node editNode;
nodePopup::nodePopup(QWidget * parent, node * n)
    : QMainWindow(parent)
{
	//homePath = GetHomePath();
	ui.setupUi(this);
	novaParent = (NovaConfig *)parent;
	editNode = *n;

	m_ethernetEdit = new HexMACSpinBox(this, editNode.MAC, macSuffix);
	m_prefixEthEdit = new HexMACSpinBox(this, editNode.MAC, macPrefix);
	ui.ethernetHBox->insertWidget(0, m_ethernetEdit);
	ui.ethernetHBox->insertWidget(0, m_prefixEthEdit);


	vector<string> profiles = novaParent->m_honeydConfig->GetProfileNames();
	int index = 0;
	for (vector<string>::iterator it = profiles .begin(); it != profiles.end(); it++)
	{
		ui.nodeProfileComboBox->addItem(QString::fromStdString(*it));

		if ((*it) == editNode.pfile)
		{
			ui.nodeProfileComboBox->setCurrentIndex(index);
		}
		index++;
	}

	LoadNode();
}

nodePopup::~nodePopup()
{

}


/************************************************
 * Load and Save changes to the node's profile
 ************************************************/

//saves the changes to a node
void nodePopup::SaveNode()
{
	editNode.MAC = m_prefixEthEdit->text().toStdString() +":"+m_ethernetEdit->text().toStdString();
	editNode.realIP = (ui.ipSpinBox0->value() << 24) +(ui.ipSpinBox1->value() << 16)
			+ (ui.ipSpinBox2->value() << 8) + (ui.ipSpinBox3->value());
	in_addr inTemp;
	inTemp.s_addr = htonl(editNode.realIP);
	editNode.IP = inet_ntoa(inTemp);
	editNode.pfile = ui.nodeProfileComboBox->currentText().toStdString();

	if (ui.isDHCP->isChecked())
	{
		editNode.IP = "DHCP";
	}

	if (ui.isRandomMAC->isChecked())
	{
		editNode.MAC = "RANDOM";
	}
}

void nodePopup::on_isDHCP_stateChanged()
{
	if (ui.isDHCP->isChecked())
	{
		ui.ipSpinBox3->setDisabled(true);
		ui.ipSpinBox2->setDisabled(true);
		ui.ipSpinBox1->setDisabled(true);
		ui.ipSpinBox0->setDisabled(true);
	}
	else
	{
		ui.ipSpinBox3->setDisabled(false);
		ui.ipSpinBox2->setDisabled(false);
		ui.ipSpinBox1->setDisabled(false);
		ui.ipSpinBox0->setDisabled(false);
	}
}

void nodePopup::on_isRandomMAC_stateChanged()
{
	if (ui.isRandomMAC->isChecked())
	{
		m_ethernetEdit->setDisabled(true);
		m_prefixEthEdit->setDisabled(true);
	}
	else
	{
		m_ethernetEdit->setDisabled(false);
		m_prefixEthEdit->setDisabled(false);
	}

}

//loads the selected node's options
void nodePopup::LoadNode()
{
	subnet s = novaParent->m_honeydConfig->m_subnets[editNode.sub];
	profile p = novaParent->m_honeydConfig->m_profiles[editNode.pfile];

	if(editNode.MAC.length() == 17)
	{
		QString prefixStr = QString(editNode.MAC.substr(0, 8).c_str()).toLower();
		prefixStr = prefixStr.remove(':');
		m_prefixEthEdit->setValue(prefixStr.toInt(NULL, 16));

		QString suffixStr = QString(editNode.MAC.substr(9, 8).c_str()).toLower();
		suffixStr = suffixStr.remove(':');
		m_ethernetEdit->setValue(suffixStr.toInt(NULL, 16));
	}
	else
	{
		m_prefixEthEdit->setValue(0);
		m_ethernetEdit->setValue(0);
	}


	ui.isRandomMAC->setChecked(editNode.MAC == "RANDOM");
	ui.isDHCP->setChecked(editNode.IP == "DHCP");

	int count = 0;
	int numBits = 32 - s.maskBits;

	in_addr_t base = ::pow(2, numBits);
	in_addr_t flatConst = ::pow(2,32)- base;

	flatConst = flatConst & editNode.realIP;
	in_addr_t flatBase = flatConst;
	count = s.maskBits/8;

	while(count < 4)
	{
		switch(count)
		{
			case 0:
			{
				ui.ipSpinBox0->setReadOnly(false);
				if(numBits > 32)
					numBits = 32;
				base = ::pow(2,32)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox0->setRange((flatBase >> 24), (flatBase+base) >> 24);
				break;
			}
			case 1:
			{
				ui.ipSpinBox1->setReadOnly(false);
				if(numBits > 24)
					numBits = 24;
				base = ::pow(2, 24)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox1->setRange(flatBase >> 16, (flatBase+base) >> 16);
				break;
			}
			case 2:
			{
				ui.ipSpinBox2->setReadOnly(false);
				if(numBits > 16)
					numBits = 16;
				base = ::pow(2, 16)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox2->setRange(flatBase >> 8, (flatBase+base) >> 8);
				break;
			}
			case 3:
			{
				ui.ipSpinBox3->setReadOnly(false);
				if(numBits > 8)
					numBits = 8;
				base = ::pow(2, 8)-1;
				flatBase = flatConst & base;
				base = ::pow(2, numBits)-1;
				ui.ipSpinBox3->setRange(flatBase, (flatBase+base));
				break;
			}
		}
		count++;
	}

	ui.ipSpinBox3->setValue(editNode.realIP & 255);
	ui.ipSpinBox2->setValue((editNode.realIP >> 8) & 255);
	ui.ipSpinBox1->setValue((editNode.realIP >> 16) & 255);
	ui.ipSpinBox0->setValue((editNode.realIP >> 24) & 255);

	ui.isRandomMAC->setChecked(editNode.MAC == "RANDOM");
	ui.isDHCP->setChecked(editNode.IP == "DHCP");
}


/************************************************
 * General GUI Signal Handling
 ************************************************/

//Closes the window without saving any changes
void nodePopup::on_cancelButton_clicked()
{
	this->close();
}

void nodePopup::on_okButton_clicked()
{
	on_applyButton_clicked();
	novaParent->LoadAllNodes();
	this->close();
}

void nodePopup::on_restoreButton_clicked()
{
	LoadNode();
}

void nodePopup::on_applyButton_clicked()
{
	NovaGUI * mainwindow  = (NovaGUI*)novaParent->parent();
	SaveNode();
	int ret = ValidateNodeSettings();
	switch(ret)
	{
		case 0:
			novaParent->m_honeydConfig->AddNewNode(editNode.pfile, editNode.IP, editNode.MAC, editNode.interface, editNode.sub);
			break;
		case 1:
			mainwindow->m_prompter->DisplayPrompt(mainwindow->NODE_LOAD_FAIL,
					"This Node requires a unique IP address");
			break;
		case 2:
			mainwindow->m_prompter->DisplayPrompt(mainwindow->NODE_LOAD_FAIL,
					"DHCP Enabled nodes requires a unique MAC Address.");
			break;
	}
	on_restoreButton_clicked();

}



void nodePopup::on_generateButton_clicked()
{
	if (ui.isRandomMAC->isChecked())
	{
		return;
	}

	editNode.MAC = novaParent->m_honeydConfig->GenerateUniqueMACAddress(novaParent->m_honeydConfig->m_profiles[editNode.pfile].ethernet);
	QString prefixStr = QString(editNode.MAC.substr(0, 8).c_str()).toLower();
	prefixStr = prefixStr.remove(':');
	m_prefixEthEdit->setValue(prefixStr.toInt(NULL, 16));

	QString suffixStr = QString(editNode.MAC.substr(9, 8).c_str()).toLower();
	suffixStr = suffixStr.remove(':');
	m_ethernetEdit->setValue(suffixStr.toInt(NULL, 16));
}

int nodePopup::ValidateNodeSettings()
{
	novaParent->m_loading->lock();

	bool ipConflict;
	bool macConflict;

	if (editNode.IP == "DHCP")
	{
		ipConflict = false;
	}
	else
	{
		ipConflict = novaParent->m_honeydConfig->IsIPUsed(editNode.IP);

		if(novaParent->m_honeydConfig->m_subnets[editNode.sub].base == editNode.realIP)
		{
			ipConflict = true;
		}
	}

	if (editNode.MAC == "RANDOM")
	{
		macConflict = false;
	}
	else
	{
		macConflict = novaParent->m_honeydConfig->IsMACUsed(editNode.MAC);
	}


	int ret = 0;
	if(ipConflict)
	{
		// TODO: Get rid of magic return numbers
		ret = 1;
	}
	else if(macConflict)
	{
		// TODO: Get rid of magic return numbers
		ret = 2;
	}
	novaParent->m_loading->unlock();
	return ret;
}
