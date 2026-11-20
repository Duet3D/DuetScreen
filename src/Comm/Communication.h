/*
 * Communication.h
 *
 *  Created on: Jan 30, 2023
 *      Author: Loïc G.
 */

#ifndef JNI_COMM_COMMUNICATION_HPP_
#define JNI_COMM_COMMUNICATION_HPP_

#include "ObjectModel/Tree/Patch.h"
#include <chrono>
#include <stdint.h>

namespace Comm
{

	enum SeqState
	{
		SeqStateInit,
		SeqStateRequested,
		SeqStateOk,
		SeqStateUpdate,
		SeqStateError,
		SeqStateDisabled
	};

	struct Seq
	{
		int32_t lastSeq;
		enum SeqState state;
		std::chrono::milliseconds lastRequestTime{0};

		const char* const key;
		const char* const flags;
	};

	extern Seq* g_currentReqSeq;

	bool GetInteger(const char s[], int32_t& rslt);
	bool GetUnsignedInteger(const char s[], uint32_t& rslt);
	bool GetFloat(const char s[], float& rslt);
	bool GetBool(const char s[], bool& rslt);
	void Reconnect();

	Seq* FindSeqByKey(const char* key);
	void UpdateSeq(std::string_view key, int32_t val);
	void ResetSeqs();

	void KickWatchdog();

	/**
	 * Merge an object model reply into the object model, noting the request it answers as done and what its `seqs` say
	 * has changed since, and asking for whatever else the reply says is still to come
	 */
	void ReceiveModelReply(const OM::Patch& patch);

	Seq* GetNextSeq(struct Seq* current);
	bool sendNext();
	void init();
} // namespace Comm

#endif /* JNI_COMM_COMMUNICATION_HPP_ */
