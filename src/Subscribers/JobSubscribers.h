#pragma once

#include "Subscribers/Subscribers.h"

class JobSubscribers : public SubscriberMap
{
  public:
	JobSubscribers()
	{
		addSubscriber("job:file:fileName", currentFileName);
		addSubscriber("job:lastFileName", lastFileName);
		addSubscriber("job:file:printTime", printTime);
		addSubscriber("job:file:simulatedTime", simulatedTime);
		addSubscriber("job:file:height", height);
		addSubscriber("job:filePosition", filePosition);
		addSubscriber("job:file:size", fileSize);
		addSubscriber("job:timesLeft:filament", filamentTimeLeft);
		addSubscriber("job:timesLeft:file", fileTimeLeft);
		addSubscriber("job:timesLeft:slicer", slicerTimeLeft);
		addSubscriber("job:warmUpDuration", warmUpDuration);
		addSubscriber("job:duration", duration);
		addSubscriber("job:build", nullBuild);
		addSubscriber("job:build:currentObject", currentObject);
		addSubscriber("job:build:objects^", nullObject);
		addSubscriber("job:build:objects^:cancelled", objectCancelled);
		addSubscriber("job:build:objects^:name", objectName);
		addSubscriber("job:build:objects^:x^", objectX);
		addSubscriber("job:build:objects^:y^", objectY);

		addArrayEndSubscriber("job:build:objects^", objectArrayEnd);
	}

  private:
	static bool currentFileName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool lastFileName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool printTime(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool simulatedTime(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool height(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool filamentTimeLeft(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool fileTimeLeft(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool slicerTimeLeft(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool filePosition(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool fileSize(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool warmUpDuration(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool duration(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool nullBuild(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool currentObject(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool nullObject(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool objectCancelled(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[]);
	static bool objectName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool objectX(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool objectY(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool objectArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
};
