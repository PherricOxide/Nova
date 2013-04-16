//============================================================================
// Name        : SuspectTable.cpp
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
// Description : Wrapper class for the SuspectHashMap object used to maintain a
// 		list of suspects.
//============================================================================

#include "ClassificationEngine.h"
#include "SerializationHelper.h"
#include "SuspectTable.h"
#include "HashMap.h"
#include "Config.h"
#include "Logger.h"
#include "Lock.h"

#include <fstream>
#include <sstream>

#define NOERROR 0

using namespace std;
using namespace Nova;

extern ClassificationEngine *engine;

namespace Nova
{
SuspectTable::SuspectTable()
{
	m_keys.clear();
	pthread_rwlockattr_t tempAttr;
	pthread_rwlockattr_init(&tempAttr);
	pthread_rwlockattr_setkind_np(&tempAttr,PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	pthread_rwlock_init(&m_lock, &tempAttr);
	pthread_mutex_init(&m_needsUpdateLock, NULL);

	m_emptySuspect.SetClassification(EMPTY_SUSPECT_CLASSIFICATION);
}

SuspectTable::~SuspectTable()
{
	//Deletes the suspects pointed to by the table
	for(SuspectHashTable::iterator it = m_suspectTable.begin(); it != m_suspectTable.end(); it++)
	{
		delete m_suspectTable[it->first];
		m_suspectTable[it->first] = NULL;
	}
	m_suspectTable.clear();
	for(SuspectLockTable::iterator it = m_lockTable.begin(); it != m_lockTable.end(); it++)
	{
		pthread_mutex_destroy(&it->second.lock);
	}
	pthread_rwlock_destroy(&m_lock);
}

//Sets the needs classification bool
void SuspectTable::SetNeedsClassificationUpdate(SuspectID_pb key)
{
	Lock updateLock(&m_needsUpdateLock);
	SetNeedsClassificationUpdate_noLocking(key);
}

void SuspectTable::SetEveryoneNeedsClassificationUpdate()
{
	Lock updateLock(&m_needsUpdateLock);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		SetNeedsClassificationUpdate_noLocking(m_keys[i]);
	}
}

//Sets the needs classification bool
void SuspectTable::SetNeedsClassificationUpdate_noLocking(SuspectID_pb key)
{
	if(!m_suspectTable[key]->m_needsClassificationUpdate)
	{
		m_suspectTable[key]->m_needsClassificationUpdate = true;
		m_suspectsNeedingUpdate.push_back(key);
	}
}

//Adds the Suspect pointed to in 'suspect' into the table using suspect->GetIPAddress() as the key;
//		suspect: pointer to the Suspect you wish to add
// Returns true on Success, and false if the suspect already exists
bool SuspectTable::AddNewSuspect(Suspect *suspect)
{
	//If we return false then there is no suspect at this ip address yet
	if(suspect != NULL)
	{
		Suspect *suspectCopy = new Suspect(*suspect);
		SuspectID_pb key = suspectCopy->GetIdentifier();
		pthread_rwlock_wrlock(&m_lock);
		if(!IsValidKey_NonBlocking(key))
		{
			//If there is already a SuspectLock this Suspect is listed for deletion but it's lock still exists
			if(m_lockTable.keyExists(key))
			{
				//Wait for the suspect lock, this call releases the table while blocking.
				LockSuspect(key);
				//Reverse the deletion flag
				m_lockTable[key].deleted = false;
				UnlockSuspect(key);
			}
			//Else we need to init a SuspectLock
			else
			{
				pthread_mutexattr_t tempAttr;
				pthread_mutexattr_init(&tempAttr);
				pthread_mutexattr_settype(&tempAttr, PTHREAD_MUTEX_ERRORCHECK);
				//Destroy before init just to be safe
				pthread_mutex_destroy(&m_lockTable[key].lock);
				pthread_mutex_init(&m_lockTable[key].lock, &tempAttr);
				m_lockTable[key].deleted = false;
				m_lockTable[key].ref_cnt = 0;
			}
			//Allocate the Suspect and copy the contents
			m_suspectTable[key] = suspectCopy;
			//Store the key
			m_keys.push_back(key);
			pthread_rwlock_unlock(&m_lock);
			return true;
		}
	}
	//Else this suspect already exists, we cannot add it again->
	pthread_rwlock_unlock(&m_lock);
	return false;
}

// Updates a suspects evidence and calculates the FeatureSet
//		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) if the call succeeds, (false) if the suspect doesn't exist or doesn't need updating
bool SuspectTable::ClassifySuspect(SuspectID_pb key)
{
	Lock lock(&m_lock, WRITE_LOCK);
	if(IsValidKey_NonBlocking(key))
	{
		Suspect *suspect = m_suspectTable[key];
		suspect->CalculateFeatures();
		engine->Classify(suspect);
		return true;
	}
	return false;
}

// Updates every suspects evidence and calculates the FeatureSet
void SuspectTable::UpdateAllSuspects()
{
	Lock lock(&m_lock, WRITE_LOCK);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			Suspect *suspect = m_suspectTable[m_keys[i]];
			suspect->CalculateFeatures();
			engine->Classify(suspect);
		}
	}
}

// Lookup and get an Asynchronous copy of the Suspect
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns an empty suspect on failure
// Note: To modify or lock a suspect use CheckOut();
// Note: This is the same as GetSuspectStatus except it copies the feature set object which can grow very large.
Suspect SuspectTable::GetSuspect(SuspectID_pb key)
{
	//Read lock the table, Suspects can only change in the table while it's write locked.
	Lock lock(&m_lock, READ_LOCK);
	if(IsValidKey_NonBlocking(key))
	{
		//Create a copy of the suspect
		Suspect ret = *m_suspectTable[key];
		return ret;
	}
	else
	{
		Suspect ret = m_emptySuspect;
		return ret;
	}
}

// Lookup and get an Asynchronous copy of the Suspect
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns an empty suspect on failure
// Note: To modify or lock a suspect use CheckOut();
// Note: This is the same as GetSuspectStatus except it copies the feature set object which can grow very large.
Suspect SuspectTable::GetShallowSuspect(SuspectID_pb key)
{
	//Read lock the table, Suspects can only change in the table while it's write locked.
	Lock lock(&m_lock, READ_LOCK);
	if(IsValidKey_NonBlocking(key))
	{
		//Create a copy of the suspect
		Suspect ret = m_suspectTable[key]->GetShallowCopy();
		return ret;
	}
	else
	{
		Suspect ret = m_emptySuspect;
		return ret;
	}
}

//Erases a suspect from the table if it is not locked
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) on success, (false) if the suspect does not exist (key is invalid)
bool SuspectTable::Erase(SuspectID_pb key)
{
	Lock lock(&m_lock, WRITE_LOCK);
	if(IsValidKey_NonBlocking(key))
	{
		//Flag as deleted
		m_lockTable[key].deleted = true;

		//Remove from key vector
		for(vector<SuspectID_pb>::iterator vit = m_keys.begin(); vit != m_keys.end(); vit++)
		{
			if(((*vit).m_ip() == key.m_ip()) && ((*vit).m_ifname() == key.m_ifname()))
			{
				m_keys.erase(vit);
				break;
			}
		}

		//Remove from suspect table
		if(m_suspectTable.keyExists(key))
		{
			delete m_suspectTable[key];
			m_suspectTable[key] = NULL;
			m_suspectTable.erase(key);
		}

		//If no threads are blocking on the lock we can destroy it.
		if(!(m_lockTable[key].ref_cnt > 0))
		{
			m_lockTable[key].ref_cnt = 0;
			pthread_mutex_destroy(&m_lockTable[key].lock);
			m_lockTable.erase(key);
		}
		return true;
	}
	return false;
}

// Clears the Suspect Table of all entries
// Note: Locks may still persist until all threads unlock or return from blocking on them.
void SuspectTable::EraseAllSuspects()
{
	Lock lock(&m_lock, WRITE_LOCK);
	m_keys.clear();

	SuspectLockTable::iterator it = m_lockTable.begin();
	while (it != m_lockTable.end())
	{
		m_lockTable[it->first].deleted = true;
		//If no threads are blocking on the lock we can destroy it.
		if(!(it->second.ref_cnt > 0))
		{
			m_lockTable[it->first].ref_cnt = 0;
			pthread_mutex_destroy(&m_lockTable[it->first].lock);
			m_lockTable.erase(it++);
		}
		else
		{
			it++;
		}
	}

	for(SuspectHashTable::iterator it = m_suspectTable.begin(); it != m_suspectTable.end(); it++)
	{
		delete m_suspectTable[it->first];
		m_suspectTable[it->first] = NULL;
	}
	m_suspectTable.clear();
}

// This function returns a vector of suspects keys the caller can iterate over to access the table.
// Returns a std::vector of every suspect currently in the table
vector<SuspectID_pb> SuspectTable::GetAllKeys()
{
	vector<SuspectID_pb> ret;
	ret.clear();
	Lock lock(&m_lock, READ_LOCK);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			ret.push_back(m_keys[i]);
		}
	}
	return ret;
}

// This function returns a vector of suspects keys the caller can iterate over to access the table.
// Returns a std::vector containing all hostile suspect keys
vector<SuspectID_pb> SuspectTable::GetKeys_of_HostileSuspects()
{
	vector<SuspectID_pb> ret;
	ret.clear();
	Lock lock(&m_lock, READ_LOCK);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			if(m_suspectTable[m_keys[i]]->GetIsHostile())
			{
				ret.push_back(m_keys[i]);
			}
		}
	}
	return ret;
}

// This function returns a vector of suspects keys the caller can iterate over to access the table.
// Returns a std::vector containing all benign suspect keys
vector<SuspectID_pb> SuspectTable::GetKeys_of_BenignSuspects()
{
	vector<SuspectID_pb> ret;
	ret.clear();
	Lock lock(&m_lock, READ_LOCK);
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			if(!m_suspectTable[m_keys[i]]->GetIsHostile())
			{
				ret.push_back(m_keys[i]);
			}
		}
	}
	return ret;
}

// This function returns a vector of suspects keys the caller can iterate over to access the table.
// Returns a std::vector containing the keys of all suspects that need a classification update.
vector<SuspectID_pb> SuspectTable::GetKeys_of_ModifiedSuspects()
{
	vector<SuspectID_pb> ret;
	vector<SuspectID_pb> invalidKeys;
	ret.clear();
	Lock lock(&m_lock, WRITE_LOCK);
	Lock updateLock(&m_needsUpdateLock);

	for(uint i = 0; i < m_suspectsNeedingUpdate.size(); i++)
	{
		if(m_lockTable.keyExists(m_suspectsNeedingUpdate[i]))
		{
			ret.push_back(m_suspectsNeedingUpdate[i]);
			m_suspectTable[m_suspectsNeedingUpdate[i]]->m_needsClassificationUpdate = false;
		}
	}

	m_suspectsNeedingUpdate.clear();
	return ret;
}



//Get the size of the Suspect Table
// Returns the size of the Table
uint SuspectTable::Size()
{
	Lock lock(&m_lock, READ_LOCK);
	uint ret = m_suspectTable.size();
	return ret;
}

void SuspectTable::SaveSuspectsToFile(string filename)
{
	Lock lock(&m_lock, READ_LOCK);
	LOG(NOTICE, "Saving Suspects...", "Saving Suspects in a text format to file "+filename);
	ofstream out(filename.c_str());
	if(!out.is_open())
	{
		LOG(ERROR,"Unable to open file requested file.",
			"Unable to open or create file located at "+filename+".");
		return;
	}
	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			out << m_suspectTable[m_keys[i]]->ToString() << endl;
		}
	}
	out.close();
}

// Checks the validity of the key - public thread-safe version
//		key: IP address of the suspect as a uint value (host byte order)
// Returns true if there is a suspect associated with the given key, false otherwise
bool SuspectTable::IsValidKey(SuspectID_pb key)
{
	Lock lock(&m_lock, READ_LOCK);
	//If we find a SuspectLock the suspect exists or is scheduled to be deleted
	return IsValidKey_NonBlocking(key);
}

bool SuspectTable::IsEmptySuspect(Suspect *suspect)
{
	return (suspect->GetClassification() == EMPTY_SUSPECT_CLASSIFICATION);
}

//Consumes the linked list of evidence objects, extracting their information and inserting them into the Suspects.
// evidence: Evidence object, if consuming more than one piece of evidence this is the start
//				of the linked list.
// Note: Every evidence object contained in the list is deallocated after use, invalidating the pointers,
//		this is a specialized function designed only for use by Consumer threads.
void SuspectTable::ProcessEvidence(Evidence *evidence, bool readOnly)
{
	// ~~~ Write lock ~~~F
	Lock lock (&m_lock, WRITE_LOCK);
	SuspectID_pb key;
	key.set_m_ip(evidence->m_evidencePacket.ip_src);
	key.set_m_ifname(evidence->m_evidencePacket.interface);

	//If this suspect doesn't have a lock, make one.
	if(!m_lockTable.keyExists(key))
	{
		pthread_mutexattr_t mAttr;
		pthread_mutexattr_init(&mAttr);
		pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_ERRORCHECK);
		m_lockTable[key] = SuspectLock();
		pthread_mutex_init(&m_lockTable[key].lock, &mAttr);
		pthread_mutexattr_destroy(&mAttr);
		m_lockTable[key].ref_cnt = 0;
	}

	//Make sure the suspect isn't flagged as deleted then grab it's lock
	m_lockTable[key].deleted = false;
	LockSuspect(key);

	//Consume and deallocate all the evidence
	//If a suspect already exists
	if(!m_suspectTable.keyExists(key))
	{
		m_keys.push_back(key);
		m_suspectTable[key] = new Suspect();

		m_suspectTable[key]->m_features.SetHaystackNodes(m_haystackNodesCached);
		m_suspectTable[key]->m_unsentFeatures.SetHaystackNodes(m_haystackNodesCached);
	}

	m_suspectTable[key]->ReadEvidence(evidence, !readOnly);

	SetNeedsClassificationUpdate(key);

	//Unlock the suspect (CheckIn)
	UnlockSuspect(key);
}

// Checks the validity of the key - private use non-locking version
//		key: IP address of the suspect as a uint value (host byte order)
// Returns true if there is a suspect associated with the given key, false otherwise
// *Note: Assumes you have already locked the table
bool SuspectTable::IsValidKey_NonBlocking(SuspectID_pb key)
{
	//If we find a SuspectLock the suspect exists or is scheduled to be deleted
	if(m_lockTable.keyExists(key))
	{
		return !m_lockTable[key].deleted;
	}
	return false;
}
//Used internally by threads to lock on a suspect
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) after locking the suspect and (false) if the suspect couldn't be found
// Note: A suspect's lock won't be deleted until there are no more threads trying to lock it
// Imporant: table MUST be write locked before calling this.
bool SuspectTable::LockSuspect(SuspectID_pb key)
{
	//If the suspect has a lock
	if(IsValidKey_NonBlocking(key))
	{
		m_lockTable[key].ref_cnt++;
		pthread_rwlock_unlock(&m_lock);
		pthread_mutex_lock(&m_lockTable[key].lock);
		pthread_rwlock_wrlock(&m_lock);
		return true;
	}
	return false;
}

//Used internally by threads when done accesses a suspect
// 		key: IP address of the suspect as a uint value (host byte order)
// Returns (true) if the Lock could be unlocked and still exists and
// (false) if the Suspect has been deleted or could not be unlocked.
// Note: automatically deletes the lock if the suspect has been deleted and the ref count is 0
bool SuspectTable::UnlockSuspect(SuspectID_pb key)
{
	//If the lock exists
	if(m_lockTable.keyExists(key))
	{
		//Attempt to unlock
		if(pthread_mutex_unlock(&m_lockTable[key].lock) != 0)
		{
			//	Failed to unlock, lock could be uninitialized or this thread may not have the lock
			//		either way return false,
			return false;
		}

		//Decrement the count if successfully unlocked
		m_lockTable[key].ref_cnt--;

		//If the Suspect has been deleted
		if(!m_lockTable[key].deleted)
		{
			//Return true, Suspect exists and is Unlocked
			return true;
		}
		//If no more threads are blocking on the lock, destroy it
		if(!(m_lockTable[key].ref_cnt > 0))
		{
			//Destroy the lock and the paired table entry
			m_lockTable[key].ref_cnt = 0;
			pthread_mutex_destroy(&m_lockTable[key].lock);
			m_lockTable.erase(key);
		}
		//Return false, Suspect doesn't exists
		return false;
	}
	//Return false, Suspect doesn't exists
	return false;
}

void SuspectTable::SetHaystackNodes(std::vector<uint32_t> nodes)
{
	Lock lock(&m_lock, WRITE_LOCK);
	Lock updateLock(&m_needsUpdateLock);

	m_haystackNodesCached = nodes;

	for(uint i = 0; i < m_keys.size(); i++)
	{
		if(IsValidKey_NonBlocking(m_keys[i]))
		{
			Suspect *suspect = m_suspectTable[m_keys[i]];

			suspect->m_features.SetHaystackNodes(nodes);
			suspect->m_unsentFeatures.SetHaystackNodes(nodes);
			SetNeedsClassificationUpdate_noLocking(m_keys[i]);
		}
	}
}

}
