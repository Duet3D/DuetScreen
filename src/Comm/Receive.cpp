/*
 * Receive.cpp
 */

#include "Receive.h"
#include "Comm/Communication.h"
#include "Comm/FileInfo.h"
#include "Comm/Replies.h"
#include "Comm/Responses.h"
#include "Debug.h"
#include "Hardware/Reset.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Tree/Json.h"
#include "ObjectModel/Tree/Patch.h"
#include <string>

namespace Comm
{
	namespace
	{
		/** Act on a control command, which the firmware sends to update or restart the screen */
		void controlCommand(std::string_view command)
		{
			if (command == "reset")
			{
				Restart(); // Does not return
			}
			else if (command == "eraseAndReset")
			{
				EraseAndRestart(); // Does not return
			}
			else
			{
				LOG_WARN("Unknown control command \"{:s}\"", command);
			}
		}
	} // namespace

	void receiveLine(std::string_view line)
	{
		auto reply = OM::parseJson(line);
		if (!reply)
		{
			LOG_WARN(
				"A reply that isn't JSON ({:s} at {:d}): {:s}", reply.error().message, reply.error().position, line);
			return;
		}
		receiveReply(*reply);
	}

	void receiveReply(OM::Node& reply)
	{
		if (auto patch = OM::Patch::fromReply(reply))
		{
			ReceiveModelReply(*patch);
			return;
		}
		KickWatchdog();

		if (reply["resp"].exists())
		{
			showResponse(reply["resp"].text());
		}
		else if (reply["message"].exists())
		{
			LOG_INFO("Message: {:s}", reply["message"].text());
		}
		else if (reply["controlCommand"].exists())
		{
			controlCommand(reply["controlCommand"].text());
		}
		else if (reply["dir"].exists())
		{
			OM::FileSystem::FileListArrived(std::string(reply["dir"].text()), FileList::read(reply));
		}
		else if (reply["thumbnail"].isObject())
		{
			FILEINFO_CACHE->ThumbnailArrived(std::string(reply["thumbnail"]["fileName"].text()),
											 FileChunk::read(reply, "thumbnail"));
		}
		else if (reply["fragment"].isObject())
		{
			OM::FileSystem::FileContentsArrived(std::string(reply["fragment"]["fileName"].text()),
												FileChunk::read(reply, "fragment"));
		}
		else if (reply["fileName"].exists())
		{
			FILEINFO_CACHE->FileInfoArrived(std::string(reply["fileName"].text()), FileInfoReply::read(reply));
		}
		else
		{
			LOG_DBG("A reply nothing is waiting for: {:s}", reply.toJson());
		}
	}
} // namespace Comm
