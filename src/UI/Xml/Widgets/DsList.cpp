/*
 * DsList.cpp
 */

#include "DsList.h"
#include "UI/Xml/View/Actions.h"
#include "WidgetSupport.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsList::DsList(lv_obj_t* container, ListBase& list, std::string item, lv_xml_subject_ctx_t* parentContext)
		: m_container(container)
		, m_list(&list)
		, m_item(std::move(item))
		, m_parentContext(lv_xml_subject_ctx_ref(parentContext))
	{
		m_list->addListener(this);
		build(0, m_list->size());
	}

	DsList::~DsList()
	{
		if (m_list)
		{
			m_list->removeListener(this);
		}
		m_entries.clear();
		lv_xml_subject_ctx_unref(m_parentContext);
	}

	void DsList::registerXml()
	{
		lv_xml_register_widget("ds_list", xmlCreate, xmlApply);
	}

	void DsList::onListResized(size_t oldSize, size_t newSize)
	{
		if (newSize > oldSize)
		{
			build(m_entries.size(), newSize);
		}
		else
		{
			truncate(newSize);
		}
	}

	void DsList::onListDestroyed()
	{
		truncate(0);
		m_list = nullptr;
	}

	void* DsList::xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
	{
		return lv_obj_create(parentOf(state));
	}

	void DsList::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		const char* listName = lv_xml_get_value_of(attrs, "list");
		const char* item = lv_xml_get_value_of(attrs, "item");
		if (listName == nullptr || item == nullptr)
		{
			LOG_WARN("<ds_list> needs both list= and item=");
			return;
		}

		lv_xml_subject_ctx_t* context = lv_xml_subject_ctx_get_active();
		ListBase* list = Scope::findList(context, listName);
		if (list == nullptr)
		{
			LOG_WARN("<ds_list list=\"{:s}\">: no such list", listName);
			return;
		}

		lv_obj_t* container = itemOf(state);
		deleteWith(container, new DsList(container, *list, item, context));
	}

	void DsList::build(size_t from, size_t to)
	{
		ZoneScoped;
		for (size_t i = from; i < to; i++)
		{
			Row& row = m_list->row(i);
			auto scope = std::make_unique<Scope>(m_parentContext);
			ScopeBinder binder(*scope);
			binder.field("index", row.index, "Position in the list");
			row.fields(binder);

			const std::string name = std::to_string(i);
			const char* attrs[] = {"name", name.c_str(), nullptr, nullptr};
			auto* obj =
				static_cast<lv_obj_t*>(lv_xml_create_with_ctx(m_container, m_item.c_str(), attrs, scope->context()));
			if (obj == nullptr)
			{
				LOG_ERROR("<ds_list>: couldn't create item component '{:s}'", m_item);
			}
			else
			{
				Actions::setRow(obj, &row);
			}
			m_entries.push_back({obj, std::move(scope)});
		}
	}

	void DsList::truncate(size_t count)
	{
		while (m_entries.size() > count)
		{
			if (m_entries.back().obj)
			{
				lv_obj_delete(m_entries.back().obj);
			}
			m_entries.pop_back();
		}
	}
} // namespace UI::Xml
