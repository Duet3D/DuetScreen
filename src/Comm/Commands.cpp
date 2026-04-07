/*
 * Commands.cpp
 *
 *  Created on: 23 Feb 2024
 *      Author: andy
 */

/*
 * Commands.h
 *
 *  Created on: 23 Feb 2024
 *      Author: andy
 */

#include "Comm/Commands.h"
#include "Debug.h"
#include "ObjectModel/Utils.h"
#include "utils/utils.h"
#include <stdlib.h>

namespace Comm
{
	// The following tables will be sorted once on startup so entries can be better grouped for code maintenance
	// A '^' character indicates the position of an _ecv_array index, and a ':' character indicates the start of a
	// sub-field name
	FieldTableEntry g_fieldTable[] = {
		// M409 common fields
		{rcvKey, "key"},
		{rcvFlags, "flags"},

		// M409 K"seqs" response
		{rcvSeqsBoards, "seqs:boards"},
		{rcvSeqsDirectories, "seqs:directories"},
		{rcvSeqsFans, "seqs:fans"},
		{rcvSeqsHeat, "seqs:heat"},
		{rcvSeqsInputs, "seqs:inputs"},
		{rcvSeqsJob, "seqs:job"},
		{rcvSeqsMove, "seqs:move"},
		{rcvSeqsNetwork, "seqs:network"},
		{rcvSeqsReply, "seqs:reply"},
		{rcvSeqsScanner, "seqs:scanner"},
		{rcvSeqsSensors, "seqs:sensors"},
		{rcvSeqsSpindles, "seqs:spindles"},
		{rcvSeqsState, "seqs:state"},
		{rcvSeqsTools, "seqs:tools"},
		{rcvSeqsVolumes, "seqs:volumes"},
		{rcvSeqsGlobal, "seqs:global"},

		// Control Command message
		{rcvControlCommand, "controlCommand"},
	};

	void SortFieldTable()
	{
		ZoneScoped;
		// Sort the g_fieldTable prior searching using binary search
		qsort(g_fieldTable, ARRAY_SIZE(g_fieldTable), sizeof(FieldTableEntry), compareKey<FieldTableEntry>);
	}

	const FieldTableEntry* SearchFieldTable(const char* id)
	{
		ZoneScoped;
		const FieldTableEntry key = {ReceivedDataEvent::rcvUnknown, id};
		const FieldTableEntry* searchResult = (FieldTableEntry*)bsearch(
			&key, g_fieldTable, ARRAY_SIZE(g_fieldTable), sizeof(FieldTableEntry), compareKey<FieldTableEntry>);
		return searchResult;
	}
} // namespace Comm
