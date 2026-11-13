/*
 * JsonDecoder.h
 *
 *  Created on: 23 Feb 2024
 *      Author: andy
 *
 *  This file is a modified version of the SerialIO.h source file from PanelDue
 *  The original can be found at https://github.com/Duet3D/PanelDueFirmware/blob/master/src/Hardware/SerialIo.h
 */

#ifndef JNI_COMM_JSONDECODER_H_
#define JNI_COMM_JSONDECODER_H_

#include "Comm/Communication.h"
#include "Comm/FileInfo.h"
#include "Configuration.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Tree/Patch.h"
#include <Duet3D/General/String.h>
#include <cstddef>
#include <memory>
#include <sys/types.h>
#include <variant>

namespace Comm
{
	class JsonDecoder
	{
	  public:
		enum class ResponseType
		{
			unknown = 0,
			filelist,
			fileInfo,
			fileContents,
			thumbnail,
		};

		// Enumeration to represent the json parsing state.
		// We don't allow nested objects or nested arrays, so we don't need a state stack.
		// An additional variable elementCount is 0 if we are not in an array, else the number of elements we have found
		// (including the current one)
		enum JsonState
		{
			jsBegin,			// initial state, expecting '{'
			jsExpectId,			// just had '{' so expecting a quoted ID
			jsId,				// expecting an identifier, or in the middle of one
			jsHadId,			// had a quoted identifier, expecting ':'
			jsVal,				// had ':', expecting value
			jsStringVal,		// had '"' and expecting or in a string value
			jsStringEscape,		// just had backslash in a string
			jsIntVal,			// receiving an integer value
			jsNegIntVal,		// had '-' so expecting a integer value
			jsFracVal,			// receiving a fractional value
			jsEndVal,			// had the end of a string or _ecv_array value, expecting comma or ] or }
			jsCharsVal,			// receiving an alphanumeric value such as true, false, null
			jsExpValSign,		// about to receive an exponent, possible sign coming up
			jsExpValFirstDigit, // expecting the first digit of an exponent
			jsExpValDigits,		// expecting remaining digits of an exponent
			jsError				// something went wrong
		};

		JsonDecoder();
		void Reset();
		void SetNextOut(size_t nextOut) { m_nextOut = nextOut; }
		void CheckInput(const unsigned char* rxBuffer, size_t len);
		void ProcessReceivedValue(StringRef id, const char val[], const size_t indices[]);
		bool SetPrefix(const char* prefix) { return m_fieldPrefix.copy(prefix); }
		const Seq* GetSeq() const { return m_seq; }

		// These variables are used for the
		void ClearResponseData()
		{
			responseType = ResponseType::unknown;
			responseData = nullptr;
		}

		bool hasFlags() const { return !m_flags.IsEmpty(); }
		size_t GetArrayStartIndex() const;

		ResponseType responseType = ResponseType::unknown;
		std::variant<void*,
					 OM::FileSystem::FileListRequestWeakPtr,
					 OM::FileSystem::FileContentsPtr,
					 FileInfoCache::FileInfoRequestPtr,
					 FileInfoCache::ThumbnailRequestPtr>
			responseData = nullptr;

	  private:
		void StartReceivedMessage(void);
		void EndReceivedMessage(void);
		void ProcessArrayEnd(const char id[], const size_t indices[]);
		void ParserErrorEncountered(int currentState, const char* id, int errors);
		void RemoveLastId();
		void RemoveLastIdChar();
		bool InArray();
		void ProcessField();
		void EndArray();
		void ConvertUnicode();
		bool CheckValueCompleted(char c, bool doProcess);
		void receiveError();

		/** Replace "result" at the start of `id` with the response's key, as subscribers know the field */
		void TranslateResultId(StringRef id) const;

		/** The JSON kind of the value just completed, from the state the parser is in */
		OM::Node::Kind ValueKind() const;

		/**
		 * Where field `id` lies in an M409 result, or nothing if it isn't in the result
		 * @param id A field id, e.g. "result:heaters^:current"
		 * @param depth How many of m_arrayIndices `id`'s '^' use
		 */
		std::optional<OM::Path> ResultPath(std::string_view id, size_t depth) const;

		/** Give a completed top-level or result value to the object model patch */
		void FeedPatch(OM::Node::Kind kind);

		// m_fieldId is the name of the field being received. A '^' character indicates the position of an _ecv_array
		// index, and a ':' character indicates a field separator.
		String<50> m_fieldPrefix;
		String<MAX_JSON_ID_LENGTH> m_fieldId;
		String<MAX_JSON_VALUE_LENGTH> m_fieldVal; // rr_thumbnail seems to be biggest response we get
		JsonState m_state = jsBegin;
		JsonState m_lastState = jsBegin;
		int m_serialIoErrors;
		size_t m_nextOut;
		bool m_inError;
		size_t m_arrayIndices[MAX_ARRAY_NESTING];
		size_t m_arrayDepth;
		Seq* m_seq = nullptr;
		String<50> m_key; // for OM responses
		String<10> m_flags; // e.g. "vna0"
		OM::Patch m_patch;	// The M409 result being received, given to OM::Tree when the message ends
	};
} // namespace Comm
#endif /* JNI_COMM_JSONDECODER_H_ */
