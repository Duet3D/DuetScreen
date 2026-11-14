/*
 * OmList.cpp
 */

#include "OmList.h"
#include "ObjectModel/Tree/Node.h"
#include "OmBindings.h"
#include "OmName.h"

namespace UI::Xml
{
	void OmRow::fields(FieldVisitor& f)
	{
		f.field("value",
				value,
				"The item as text, if it isn't an object (e.g. a heater number); `value|int` etc. "
				"convert it (see Converters)");
		f.field("exists",
				exists,
				"1 unless the item is null, so `<bind_flag_if_eq subject=\"exists\" ref_value=\"0\" "
				"flag=\"hidden\"/>` hides the gaps");
		f.resolver(*this);
	}

	void OmRow::show(OmBindings& bindings, const OM::Path& path, const OM::Node& node)
	{
		m_bindings = &bindings;
		m_path = path;
		value.set(node.isValue() ? node.text() : "");
		exists.set(node.exists() ? 1 : 0);
	}

	Subject* OmRow::findSubject(const lv_xml_subject_ctx_t* /* origin */, std::string_view name, bool last)
	{
		const auto parsed = OmName::parse(name);
		if (!parsed || parsed->absolute || m_bindings == nullptr)
		{
			return nullptr;
		}
		std::optional<OM::Path> path;
		if (parsed->path == "value")
		{
			// `value` itself is a field of the row; `value|int` converts the item
			if (parsed->converter.empty())
			{
				return nullptr;
			}
			path = m_path;
		}
		else
		{
			path = resolve(parsed->path, last);
		}
		return path ? m_bindings->subject(*path, parsed->converter, parsed->argument) : nullptr;
	}

	ListBase* OmRow::findList(const lv_xml_subject_ctx_t* /* origin */, std::string_view name, bool last)
	{
		const auto parsed = OmName::parse(name);
		if (!parsed || parsed->absolute || !parsed->converter.empty() || m_bindings == nullptr)
		{
			return nullptr;
		}
		const auto path = resolve(parsed->path, last);
		return path ? m_bindings->list(*path) : nullptr;
	}

	std::optional<OM::Path> OmRow::resolve(std::string_view relative, bool last) const
	{
		const auto path = OM::Path::parse(relative);
		if (!path || path->empty())
		{
			return std::nullopt;
		}
		const auto& first = std::get<std::string>(path->segments().front());
		if (!last && !m_bindings->has(m_path.field(first)))
		{
			return std::nullopt;
		}
		return m_path / *path;
	}

	OmList::OmList(OmBindings& bindings, OM::Path path)
		: m_bindings(bindings)
		, m_path(std::move(path))
	{
	}

	void OmList::refresh(const OM::Node& root)
	{
		const OM::Node& node = root.at(m_path);
		sync(node.isArray() ? node.size() : 0,
			 [&](OmRow& row, size_t i) { row.show(m_bindings, m_path.index(i), node[i]); });
	}
} // namespace UI::Xml
