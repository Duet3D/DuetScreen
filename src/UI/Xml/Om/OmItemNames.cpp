/*
 * OmItemNames.cpp
 */

#include "OmItemNames.h"
#include "Debug.h"
#include "OmBindings.h"
#include "OmName.h"

namespace UI::Xml
{
	void OmItemNames::setItem(OmBindings& bindings, OmBoundPath item)
	{
		m_bindings = &bindings;
		m_item = std::move(item);
	}

	Subject* OmItemNames::findSubject(const lv_xml_subject_ctx_t* origin, std::string_view name, bool last)
	{
		const auto parsed = OmName::parse(name);
		if (!parsed || parsed->absolute || m_bindings == nullptr)
		{
			return nullptr;
		}
		std::optional<OmBoundPath> path;
		if (parsed->path == "value")
		{
			// `value|int` converts the item itself; `value` alone is a row's own field
			if (parsed->converter.empty())
			{
				return nullptr;
			}
			path = m_item;
		}
		else
		{
			path = resolve(origin, parsed->path, last);
		}
		return path ? m_bindings->subjectFor(*path, parsed->converter, parsed->argument, origin, name) : nullptr;
	}

	ListBase* OmItemNames::findList(const lv_xml_subject_ctx_t* origin, std::string_view name, bool last)
	{
		const auto parsed = OmName::parse(name);
		if (!parsed || parsed->absolute || !parsed->converter.empty() || m_bindings == nullptr)
		{
			return nullptr;
		}
		const auto path = resolve(origin, parsed->path, last);
		return path ? m_bindings->listFor(*path, origin, name) : nullptr;
	}

	std::optional<OmBoundPath> OmItemNames::resolve(const lv_xml_subject_ctx_t* origin,
													std::string_view relative,
													bool last) const
	{
		const auto relativePath = OmTemplate::parse(relative);
		if (!relativePath)
		{
			return std::nullopt;
		}
		if (!last)
		{
			const auto item = m_item.resolve();
			if (!item || !m_bindings->has(item->field(relativePath->firstField())))
			{
				return std::nullopt;
			}
		}
		std::string missing;
		const auto bound = OmBoundPath::bind(
			*relativePath, [origin](std::string_view index) { return lookupSubject(origin, index); }, missing);
		if (!bound)
		{
			LOG_WARN("Object model name '{:s}': no subject '{:s}' for its index", relative, missing);
			return std::nullopt;
		}
		return m_item / *bound;
	}
} // namespace UI::Xml
