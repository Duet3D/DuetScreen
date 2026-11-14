/*
 * OmField.cpp
 */

#include "OmField.h"
#include "ObjectModel/Tree/Node.h"

namespace UI::Xml
{
	OmField::OmField(OM::Path path, const Converter& converter, std::string argument)
		: m_path(std::move(path))
		, m_converter(converter)
		, m_argument(std::move(argument))
	{
		if (m_converter.type == Converter::Type::integer)
		{
			m_integer = std::make_unique<IntSubject>();
		}
		else
		{
			m_text = std::make_unique<StringSubject<TEXT_SIZE>>();
		}
	}

	Subject& OmField::subject()
	{
		if (m_integer)
		{
			return *m_integer;
		}
		return *m_text;
	}

	void OmField::update(const OM::Node& root)
	{
		const OM::Node& node = root.at(m_path);
		if (m_integer)
		{
			m_integer->set(m_converter.toInteger(node, m_argument));
		}
		else
		{
			m_text->set(m_converter.toText(node, m_argument));
		}
	}
} // namespace UI::Xml
