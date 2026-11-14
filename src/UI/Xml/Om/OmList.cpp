/*
 * OmList.cpp
 */

#include "OmList.h"
#include "ObjectModel/Tree/Node.h"
#include "OmBindings.h"

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
		f.resolver(m_names);
	}

	void OmRow::show(OmBindings& bindings, const OM::Path& path, const OM::Node& node)
	{
		m_names.setItem(bindings, OmBoundPath(path));
		value.set(node.isValue() ? node.text() : "");
		exists.set(node.exists() ? 1 : 0);
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
