/*
 * Replies.cpp
 */

#include "Replies.h"
#include "tracy/Tracy.hpp"
#include <fmt/format.h>

namespace Comm
{
	namespace
	{
		/** `node` as a count: a whole number of 0 or more, or `fallback` */
		template <typename T>
		T count(const OM::Node& node, T fallback = 0)
		{
			ZoneScoped;
			const int64_t value = node.integer(-1);
			return value < 0 ? fallback : static_cast<T>(value);
		}

		/** A ReplyError for a reply without the field `field` */
		ReplyError missing(std::string_view what, std::string_view field)
		{
			ZoneScoped;
			return ReplyError{.code = 0, .message = fmt::format("{:s} without \"{:s}\"", what, field)};
		}

		/** Read one item of a folder */
		FileEntry readEntry(const OM::Node& item)
		{
			ZoneScoped;
			return FileEntry{
				.folder = item["type"].text() == "d",
				.name = std::string(item["name"].text()),
				.size = count<uint64_t>(item["size"]),
				.date = std::string(item["date"].text()),
			};
		}
	} // namespace

	std::optional<ReplyError> firmwareError(const OM::Node& reply)
	{
		ZoneScoped;
		const int64_t err = reply["err"].integer(0);
		if (err == 0)
		{
			return std::nullopt;
		}
		return ReplyError{.code = static_cast<int32_t>(err), .message = fmt::format("the firmware's error {:d}", err)};
	}

	std::expected<FileList, ReplyError> FileList::read(const OM::Node& reply, std::string_view dir)
	{
		ZoneScoped;
		FileList list;
		if (reply.isArray())
		{
			list.dir = dir;
			for (size_t i = 0; i < reply.size(); i++)
			{
				list.files.push_back(readEntry(reply[i]));
			}
			return list;
		}
		if (const auto error = firmwareError(reply))
		{
			return std::unexpected(*error);
		}
		if (!reply["files"].isArray())
		{
			return std::unexpected(missing("A folder listing", "files"));
		}
		list.dir = reply["dir"].text();
		list.first = count<size_t>(reply["first"]);
		list.next = count<size_t>(reply["next"]);
		const OM::Node& files = reply["files"];
		for (size_t i = 0; i < files.size(); i++)
		{
			list.files.push_back(readEntry(files[i]));
		}
		return list;
	}

	std::expected<FileInfo, ReplyError> FileInfo::read(const OM::Node& reply)
	{
		ZoneScoped;
		if (const auto error = firmwareError(reply))
		{
			return std::unexpected(*error);
		}
		if (reply["fileName"].kind() != OM::Node::Kind::string)
		{
			return std::unexpected(missing("File information", "fileName"));
		}
		FileInfo info;
		info.fileName = reply["fileName"].text();
		info.size = count<uint64_t>(reply["size"]);
		info.lastModified = reply["lastModified"].text();
		info.height = reply["height"].number();
		info.layerHeight = reply["layerHeight"].number();
		info.numLayers = count<uint32_t>(reply["numLayers"]);
		info.printTime = count<uint32_t>(reply["printTime"]);
		info.simulatedTime = count<uint32_t>(reply["simulatedTime"]);
		info.generatedBy = reply["generatedBy"].text();
		const OM::Node& filament = reply["filament"];
		for (size_t i = 0; i < filament.size(); i++)
		{
			info.filament.push_back(filament[i].number());
		}
		const OM::Node& thumbnails = reply["thumbnails"];
		for (size_t i = 0; i < thumbnails.size(); i++)
		{
			const OM::Node& thumbnail = thumbnails[i];
			info.thumbnails.push_back(ThumbnailInfo{
				.format = std::string(thumbnail["format"].text()),
				.width = count<uint32_t>(thumbnail["width"]),
				.height = count<uint32_t>(thumbnail["height"]),
				.offset = count<uint32_t>(thumbnail["offset"]),
				.size = count<uint32_t>(thumbnail["size"]),
			});
		}
		return info;
	}

	std::expected<FileChunk, ReplyError> FileChunk::read(const OM::Node& reply, std::string_view wrapper)
	{
		ZoneScoped;
		const OM::Node& fields = reply[wrapper].isObject() ? reply[wrapper] : reply;
		if (const auto error = firmwareError(fields))
		{
			return std::unexpected(*error);
		}
		if (fields["data"].kind() != OM::Node::Kind::string)
		{
			return std::unexpected(missing("A piece of a file", "data"));
		}
		return FileChunk{
			.fileName = std::string(fields["fileName"].text()),
			.offset = count<uint32_t>(fields["offset"]),
			.data = std::string(fields["data"].text()),
			.next = count<uint32_t>(fields["next"]),
		};
	}
} // namespace Comm
