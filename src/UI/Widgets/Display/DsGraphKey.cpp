/*
 * DsGraphKey.cpp
 */

#include "DsGraphKey.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Binding/Scope.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		constexpr int32_t TAP_REACH = 12; // How far around the key a tap still reaches it, px: it is small to aim at
	} // namespace

	DsGraphKey::DsGraphKey(const std::string& name, lv_obj_t* parent)
		: LvContainer(name, parent)
	{
		const auto& lvgl = Themes::getLvglStyles();
		const auto& components = Themes::getComponentStyles();

		// Drawn as the legend draws its keys: hollow while the series is hidden
		addStyle(components.graph_legend_swatch);
		addStyle(components.graph_legend_swatch_shown, LV_STATE_CHECKED);
		addStyle(lvgl.pressed, LV_STATE_PRESSED);
		setFlag(LV_OBJ_FLAG_CLICKABLE, true);
		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
		lv_obj_set_ext_click_area(getRootPtr(), TAP_REACH);
		addEventCallback(clickedCb, LV_EVENT_CLICKED, this);
		setVisible(false);

		m_themeListener = Themes::addThemeListener([this] { refresh(); });
	}

	DsGraphKey::~DsGraphKey()
	{
		Themes::removeThemeListener(m_themeListener);
		forgetHistory();
	}

	void DsGraphKey::registerXml()
	{
		Widgets::add(
			{.name = "ds_graph_key",
			 .extends = "lv_obj",
			 .description =
				 "One series' colour key, drawn as a graph's legend draws it, to put beside what the series "
				 "measures (e.g. a heater) so it can be matched to its line in a `<ds_graph>` of the same "
				 "history. The series is the history row whose `row.key` is the key, so it is found however "
				 "the series are ordered, and its colour follows the theme as the graph's does. Tapping it "
				 "shows or hides the series in every graph of the history; it is hollow while hidden. It "
				 "hides itself while no series has the key, and while `enabled` is false (it sets its own "
				 "`hidden` flag, so `enabled` is how to turn it off). Styled by the theme's graph_legend_swatch "
				 "and graph_legend_swatch_shown; a tap reaches it from a little way around it "
				 "(`ext_click_area`).",
			 .example = R"(<ds_graph_key history="source.temperature.temperatures" key="1"/>)",
			 .attributes = {{"history",
							 "The history the series is in (required), named like a list, e.g. "
							 "`source.temperature.temperatures`",
							 AttributeKind::List},
							{"key", "The series' row.key, e.g. a sensor number"},
							{"bind_key",
							 "Follow an int subject holding the series' row.key, e.g. "
							 "`om.heat.heaters[{row.value}].sensor|int` for a heater's series in "
							 "source.temperature.temperatures",
							 AttributeKind::Subject},
							{"enabled",
							 "`false` hides the key whatever the series, e.g. to leave it to a widget setting "
							 "(default true)"}}},
			xmlCreate,
			xmlApply);
	}

	void DsGraphKey::showHistory(History& history)
	{
		forgetHistory();
		m_history = &history;
		history.addListener(this);
		observeRows(history.size());
		refresh();
	}

	void DsGraphKey::setKey(int32_t key)
	{
		m_key = key;
		refresh();
	}

	void DsGraphKey::setEnabled(bool enabled)
	{
		m_enabled = enabled;
		refresh();
	}

	void* DsGraphKey::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* key = new DsGraphKey(attrOr(attrs, "name", "graph_key"), parentOf(state));
		deleteWith(key->getRootPtr(), key);
		return key->getRootPtr();
	}

	void DsGraphKey::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& key = static_cast<DsGraphKey&>(*LvObj::fromPtr(root));

		if (const char* historyName = lv_xml_get_value_of(attrs, "history"))
		{
			auto* history = dynamic_cast<History*>(Scope::findList(lv_xml_subject_ctx_get_active(), historyName));
			if (history == nullptr)
			{
				LOG_WARN("<ds_graph_key history=\"{:s}\">: no such history", historyName);
			}
			else
			{
				key.showHistory(*history);
			}
		}
		if (const char* enabled = lv_xml_get_value_of(attrs, "enabled"))
		{
			key.setEnabled(lv_xml_to_bool(enabled));
		}
		if (const char* value = lv_xml_get_value_of(attrs, "key"))
		{
			key.setKey(lv_xml_atoi(value));
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_key"))
		{
			if (subject->type != LV_SUBJECT_TYPE_INT)
			{
				LOG_WARN("<ds_graph_key bind_key=\"{:s}\">: not an int subject; add |int",
						 lv_xml_get_value_of(attrs, "bind_key"));
				return;
			}
			lv_subject_add_observer_obj(subject, keyObserverCb, root, &key);
		}
	}

	void DsGraphKey::keyObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsGraphKey*>(lv_observer_get_user_data(observer))->setKey(lv_subject_get_int(subject));
	}

	void DsGraphKey::rowObserverCb(lv_observer_t* observer, lv_subject_t*)
	{
		static_cast<DsGraphKey*>(lv_observer_get_user_data(observer))->refresh();
	}

	void DsGraphKey::clickedCb(lv_event_t* e)
	{
		auto* key = static_cast<DsGraphKey*>(lv_event_get_user_data(e));
		if (const auto index = key->findSeries())
		{
			IntSubject& shown = key->m_history->row(*index).shown;
			shown.set(shown.get() != 0 ? 0 : 1);
		}
	}

	void DsGraphKey::onListResized(size_t, size_t newSize)
	{
		// Added rows are already filled in; removed ones are still there, to let go of before they go
		observeRows(newSize);
		refresh();
	}

	void DsGraphKey::onListDestroyed()
	{
		// The history is going: only let go of its rows, don't call back into it
		observeRows(0);
		m_history = nullptr;
		refresh();
	}

	std::optional<size_t> DsGraphKey::findSeries() const
	{
		// Only the rows observed: while the history shrinks, it still holds the rows being removed
		for (size_t i = 0; i < m_rows.size(); i++)
		{
			if (m_history->row(i).key.get() == m_key)
			{
				return i;
			}
		}
		return std::nullopt;
	}

	void DsGraphKey::refresh()
	{
		const auto index = findSeries();
		setVisible(m_enabled && index.has_value());
		if (!index)
		{
			return;
		}
		const lv_color_t color = Themes::seriesColor(*index);
		lv_obj_t* root = getRootPtr();
		lv_obj_set_style_bg_color(root, color, LV_PART_MAIN);
		lv_obj_set_style_border_color(root, color, LV_PART_MAIN);
		setState(LV_STATE_CHECKED, m_history->row(*index).shown.get() != 0);
	}

	void DsGraphKey::observeRows(size_t count)
	{
		while (m_rows.size() > count)
		{
			lv_observer_remove(m_rows.back().key);
			lv_observer_remove(m_rows.back().shown);
			m_rows.pop_back();
		}
		while (m_rows.size() < count)
		{
			SeriesRow& row = m_history->row(m_rows.size());
			m_rows.push_back({.key = lv_subject_add_observer(row.key.raw(), rowObserverCb, this),
							  .shown = lv_subject_add_observer(row.shown.raw(), rowObserverCb, this)});
		}
	}

	void DsGraphKey::forgetHistory()
	{
		if (m_history == nullptr)
		{
			return;
		}
		observeRows(0);
		m_history->removeListener(this);
		m_history = nullptr;
	}
} // namespace UI::Xml
