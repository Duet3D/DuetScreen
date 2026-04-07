/*
 * Commands.h
 *
 *  Created on: 23 Feb 2024
 *      Author: andy
 */

#ifndef JNI_COMM_COMMANDS_H_
#define JNI_COMM_COMMANDS_H_

namespace Comm
{
	enum ReceivedDataEvent
	{
		rcvUnknown = 0,

		// Keys for control command messages
		rcvControlCommand,

		// Keys for M409 response
		rcvKey,
		rcvFlags,
		rcvResult,

		// Keys for seqs response
		rcvSeqsBoards,
		rcvSeqsDirectories,
		rcvSeqsFans,
		rcvSeqsHeat,
		rcvSeqsInputs,
		rcvSeqsJob,
		rcvSeqsLimits,
		rcvSeqsMove,
		rcvSeqsNetwork,
		rcvSeqsReply,
		rcvSeqsScanner,
		rcvSeqsSensors,
		rcvSeqsSpindles,
		rcvSeqsState,
		rcvSeqsTools,
		rcvSeqsVolumes,
		rcvSeqsGlobal,
		rcvSeqsFreq,
	};

	struct FieldTableEntry
	{
		const ReceivedDataEvent val;
		const char* key;
	};

	// The following tables will be sorted once on startup so entries can be better grouped for code maintenance
	// A '^' character indicates the position of an _ecv_array index, and a ':' character indicates the start of a
	// sub-field name
	// Needs to be sorted alphabetically by key for binary search to work
	extern FieldTableEntry g_fieldTable[];

	void SortFieldTable();
	const FieldTableEntry* SearchFieldTable(const char* id);
} // namespace Comm

#endif /* JNI_COMM_COMMANDS_H_ */
