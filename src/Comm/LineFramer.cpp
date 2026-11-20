/*
 * LineFramer.cpp
 */

#include "LineFramer.h"
#include "Debug.h"

namespace Comm
{
	LineFramer::LineFramer(LineHandler handler)
		: m_handler(std::move(handler))
	{
	}

	void LineFramer::push(std::string_view bytes)
	{
		while (!bytes.empty())
		{
			const size_t end = bytes.find('\n');
			const std::string_view piece = bytes.substr(0, end);
			if (!m_dropping)
			{
				if (m_line.size() + piece.size() > maxLineLength)
				{
					LOG_ERROR("A line longer than {:d} bytes was received; it is dropped", maxLineLength);
					m_line.clear();
					m_dropping = true;
				}
				else
				{
					m_line.append(piece);
				}
			}
			if (end == std::string_view::npos)
			{
				return;
			}
			endLine();
			bytes.remove_prefix(end + 1);
		}
	}

	void LineFramer::reset()
	{
		m_line.clear();
		m_dropping = false;
	}

	void LineFramer::endLine()
	{
		if (!m_line.empty() && m_line.back() == '\r')
		{
			m_line.pop_back();
		}
		if (!m_dropping && !m_line.empty())
		{
			m_handler(m_line);
		}
		m_line.clear();
		m_dropping = false;
	}
} // namespace Comm
