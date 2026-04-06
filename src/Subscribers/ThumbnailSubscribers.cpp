/*
 * FanSubscribers.cpp
 *
 *  Created on: 4 Jan 2024
 *      Author: Andy Everitt
 */
#include "Debug.h"

#include "Comm/FileInfo.h"
#include "Configuration.h"
#include "ThumbnailSubscribers.h"
#include "UI/Core/Model.h"
#include "nameof.hpp"

bool ThumbnailSubscribers::fileName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	// TODO this is not thread safe with multiple parallel requests
	FILEINFO_CACHE->ReceivingFileInfoResponse(data);
	Comm::FileInfoCache::FileInfoRequestPtr request = FILEINFO_CACHE->GetFileInfoRequest(data);
	if (!request)
	{
		// Should be impossible to get here
		LOG_WARN("FileInfo not found");
		return false;
	}
	decoder->responseType = Comm::JsonDecoder::ResponseType::fileInfo;
	decoder->responseData = request;

	LOG_DBG("Receiving file info about {:s}", request->GetData()->filename.c_str());
	return true;
}

bool ThumbnailSubscribers::lastModified(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	LOG_DBG("lastModified {:s}", data);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("lastModified received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->lastModified.copy(data);
	return true;
}

bool ThumbnailSubscribers::size(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("size received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->size = data;
	return true;
}

bool ThumbnailSubscribers::printTime(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("printTime received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->printTime = data;
	return true;
}

bool ThumbnailSubscribers::simulatedTime(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("simulatedTime received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->printTime = data;
	return true;
}

bool ThumbnailSubscribers::height(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("height received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->height = data;
	return true;
}

bool ThumbnailSubscribers::layerHeight(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("layerHeight received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->layerHeight = data;
	return true;
}

bool ThumbnailSubscribers::filament(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("filament received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->filament.resize(indices[0] + 1);
	fileInfo->filament[indices[0]] = data;
	return true;
}

bool ThumbnailSubscribers::thumbnailsFormat(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	LOG_VERBOSE("thumbnail format {:s}", data);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("thumbnailsFormat received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	Comm::ThumbnailPtr thumbnail = fileInfo->GetOrCreateThumbnail(indices[0]);
	if (!thumbnail->meta.SetImageFormat(data))
	{
		LOG_WARN("Thumbnail format invalid");
	}
	return true;
}

bool ThumbnailSubscribers::thumbnailsHeight(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	LOG_VERBOSE("thumbnail height {:d}", data);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("thumbnailsHeight received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->GetOrCreateThumbnail(indices[0])->meta.height = data;
	return true;
}

bool ThumbnailSubscribers::thumbnailsOffset(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	LOG_VERBOSE("thumbnail offset {:d}", data);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("thumbnailsOffset received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	Comm::ThumbnailPtr thumbnail = fileInfo->GetOrCreateThumbnail(indices[0]);
	thumbnail->meta.offset = data;
	thumbnail->context.next = data;
	return true;
}

bool ThumbnailSubscribers::thumbnailsSize(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	LOG_VERBOSE("thumbnail size {:d}", data);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("thumbnailsSize received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->GetOrCreateThumbnail(indices[0])->meta.size = data;
	return true;
}

bool ThumbnailSubscribers::thumbnailsWidth(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	LOG_VERBOSE("thumbnail width {:d}", data);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("thumbnailsWidth received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->GetOrCreateThumbnail(indices[0])->meta.width = data;
	return true;
}

bool ThumbnailSubscribers::generatedBy(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		LOG_DBG("lastModified received but not in fileInfo response");
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
	{
		LOG_WARN("FileInfo not found");
		return false;
	}
	fileInfo->generatedBy.copy(data == nullptr ? "" : data);
	FILEINFO_CACHE->FileInfoRequestComplete(fileInfo->filename.c_str());
	return true;
}

bool ThumbnailSubscribers::thumbnailFilename(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	Comm::FileInfoCache::ThumbnailRequestPtr request = FILEINFO_CACHE->GetThumbnailRequest(data);
	if (!request)
	{
		LOG_WARN("Not expecting to receive thumbnail data for \"{:s}\", ignoring", data);
		return false;
	}

	decoder->responseType = Comm::JsonDecoder::ResponseType::thumbnail;
	decoder->responseData = request;

	Comm::ThumbnailPtr thumbnail = request->GetData();
	if (thumbnail == nullptr)
	{
		LOG_ERROR("Invalid thumbnail");
		return false;
	}
	return true;
}

static bool getThumbnailFromDecoder(Comm::JsonDecoder* decoder,
									Comm::FileInfoCache::ThumbnailRequestPtr& request,
									Comm::ThumbnailPtr& thumbnail)
{
	ZoneScoped;
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::thumbnail)
	{
		return false;
	}

	try
	{
		request = std::get<Comm::FileInfoCache::ThumbnailRequestPtr>(decoder->responseData);
	}
	catch (const std::bad_variant_access&)
	{
		return false;
	}
	if (!request)
	{
		LOG_ERROR("Not expecting to receive thumbnail data");
		return false;
	}

	thumbnail = request->GetData();
	if (thumbnail == nullptr)
	{
		LOG_ERROR("Invalid thumbnail");
		return false;
	}
	return true;
}

bool ThumbnailSubscribers::thumbnailOffset(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	Comm::FileInfoCache::ThumbnailRequestPtr request;
	Comm::ThumbnailPtr thumbnail;
	if (!getThumbnailFromDecoder(decoder, request, thumbnail))
	{
		return false;
	}

	uint32_t offset = 0;
	if (!Comm::GetUnsignedInteger(data, offset))
	{
		LOG_WARN("thumbnail offset error \"{:s}\"", data);
		thumbnail->context.parseErr = Comm::ThumbnailContext::ParseErr::InvalidOffset;
		return false;
	}
	if (offset != thumbnail->context.next)
	{
		LOG_WARN("thumbnail offset mismatch, expected {:d} got {:d}", thumbnail->context.next, offset);
		thumbnail->context.parseErr = Comm::ThumbnailContext::ParseErr::MismatchOffset;
		decoder->ClearResponseData();
		request->Complete(true);
		return false;
	}

	LOG_DBG("Receiving thumbnail information about {:s}", thumbnail->filename.c_str());
	request->Receiving();

	LOG_DBG("thumbnail receive current offset {:d}.", thumbnail->context.offset);
	return true;
}

bool ThumbnailSubscribers::thumbnailData(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	Comm::FileInfoCache::ThumbnailRequestPtr request;
	Comm::ThumbnailPtr thumbnail;
	if (!getThumbnailFromDecoder(decoder, request, thumbnail))
	{
		return false;
	}

	LOG_DBG("thumbnail data {:d}", strlen(data));
	Comm::ThumbnailBuf& thumbnailBuf = request->GetBuffer();
	thumbnailBuf.size = static_cast<uint16_t>(strnlen(data, sizeof(thumbnailBuf.buffer)));
	memcpy(thumbnailBuf.buffer, data, thumbnailBuf.size);

	thumbnail->context.state = Comm::ThumbnailState::Data;
	return true;
}

bool ThumbnailSubscribers::thumbnailNext(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	Comm::FileInfoCache::ThumbnailRequestPtr request;
	Comm::ThumbnailPtr thumbnail;
	if (!getThumbnailFromDecoder(decoder, request, thumbnail))
	{
		return false;
	}

	if (!Comm::GetUnsignedInteger(data, thumbnail->context.next))
	{
		LOG_WARN("thumbnail next error \"{:s}\"", data);
		thumbnail->context.parseErr = Comm::ThumbnailContext::ParseErr::InvalidNext;
		return false;
	}
	LOG_DBG("thumbnail next {:d}", thumbnail->context.next);
	return true;
}

bool ThumbnailSubscribers::thumbnailErr(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(indices);
	Comm::FileInfoCache::ThumbnailRequestPtr request;
	Comm::ThumbnailPtr thumbnail;
	if (!getThumbnailFromDecoder(decoder, request, thumbnail))
	{
		return false;
	}

	if (!Comm::GetInteger(data, thumbnail->context.err))
	{
		LOG_WARN("Failed to parse thumbnail err {:s}", data);
		thumbnail->context.parseErr = Comm::ThumbnailContext::ParseErr::RrfError;
	}
	LOG_DBG("Thumbnail: offset({:d}), next({:d}), err({:d}), size({:d}), parseErr({:s})",
			thumbnail->context.offset,
			thumbnail->context.next,
			thumbnail->context.err,
			thumbnail->context.size,
			nameof::nameof_enum(thumbnail->context.parseErr));

	if (thumbnail->context.parseErr != Comm::ThumbnailContext::ParseErr::NoError || thumbnail->context.err != 0)
	{
		LOG_ERROR("thumbnail parseErr {:s} err {:d}.\n",
				  nameof::nameof_enum(thumbnail->context.parseErr),
				  thumbnail->context.err);
		request->Complete(true);
		return false;
	}

	if (thumbnail->context.state != Comm::ThumbnailState::Data)
	{
		LOG_WARN("No thumbnail data received for \"{:s}\"", thumbnail->filename.c_str());
		return false;
	}

	Comm::ThumbnailBuf& thumbnailBuf = request->GetBuffer();
	if (!ThumbnailDataIsValid(thumbnailBuf))
	{
		LOG_ERROR("thumbnail meta or data invalid.\n");
		request->Complete(true);
		return false;
	}

	int ret = ThumbnailDecodeChunk(*thumbnail, thumbnailBuf);
	if (ret < 0)
	{
		LOG_ERROR("failed to decode thumbnail chunk {:d}.\n", ret);
		request->Complete(true);
		return false;
	}
	if (thumbnail->context.next == 0)
	{
		thumbnail->context.state = Comm::ThumbnailState::Cached;
	}
	else
	{
		thumbnail->context.state = Comm::ThumbnailState::DataRequest;
	}
	return true;
}

bool ThumbnailSubscribers::thumbnailsArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	LOG_DBG("Thumbnail array end");
	if (decoder->responseType != Comm::JsonDecoder::ResponseType::fileInfo)
	{
		return false;
	}
	auto request = std::get<Comm::FileInfoCache::FileInfoRequestPtr>(decoder->responseData);
	if (!request)
	{
		return false;
	}
	Comm::FileInfoPtr fileInfo = request->GetData();
	if (!fileInfo)
		return false;
	fileInfo->ClearThumbnails(indices[0]); // remove any extra thumbnails (ie if the file has been overwritten with
										   // fewer thumbnails)
	if (!FILEINFO_CACHE->IsThumbnailCached(fileInfo->filename.c_str(), fileInfo->lastModified.c_str()))
	{
		FILEINFO_CACHE->QueueThumbnailRequest(fileInfo->filename.c_str());
	}
	LOG_INFO("FileInfo: filename({:s}) thumbnails({:d})", fileInfo->filename.c_str(), fileInfo->GetThumbnailCount());
	for (size_t i = 0; i < fileInfo->GetThumbnailCount(); i++)
	{
		Comm::ThumbnailPtr thumbnail = fileInfo->GetThumbnail(i);
		LOG_DBG("Thumbnail {:d}: filename({:s}) offset({:d}) size({:d}) width({:d}) height({:d}) format({:d})",
				i,
				thumbnail->filename.c_str(),
				thumbnail->meta.offset,
				thumbnail->meta.size,
				thumbnail->meta.width,
				thumbnail->meta.height,
				(int)thumbnail->meta.imageFormat);
	}
	return true;
}
