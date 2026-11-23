/*
 * ResponseSource.cpp
 */

#include "ResponseSource.h"
#include "Comm/Responses.h"

namespace UI::Xml
{
	ResponseSource::ResponseSource()
		: DataSource("response")
	{
	}

	void ResponseSource::fields(FieldVisitor& f)
	{
		f.field("count", m_count, "Number of responses received; changes with every response");
	}

	void ResponseSource::onInit()
	{
		registerEventListener<EventType::Response>(this, &ResponseSource::received);
	}

	void ResponseSource::received(const ResponseType, const std::string&)
	{
		m_count.set(m_count.get() + 1);
	}
} // namespace UI::Xml
