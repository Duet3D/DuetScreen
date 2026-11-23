/*
 * Responses.cpp
 */

#include "Responses.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "tracy/Tracy.hpp"
#include <string>

namespace Comm
{
	void showResponse(std::string_view text)
	{
		ZoneScoped;
		while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
		{
			text.remove_suffix(1);
		}
		if (text.empty())
		{
			return;
		}
		LOG_DBG("Response: {:s}", text);

		ResponseType type = ResponseType::INFO;
		if (text.starts_with("Error: "))
		{
			type = ResponseType::ERROR;
		}
		else if (text.starts_with("Warning: "))
		{
			type = ResponseType::WARNING;
		}
		Model::get().post<EventType::Response>(type, std::string(text));
	}
} // namespace Comm
