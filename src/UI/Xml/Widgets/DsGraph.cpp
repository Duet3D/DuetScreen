/*
 * DsGraph.cpp
 */

#include "DsGraph.h"
#include "UI/Xml/Binding/Scope.h"
#include "WidgetSupport.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsGraph::DsGraph(const std::string& name, lv_obj_t* parent)
		: Graph(name, parent)
	{
	}

	DsGraph::~DsGraph()
	{
		forgetHistory();
	}

	void DsGraph::registerXml()
	{
		lv_xml_register_widget("ds_graph", xmlCreate, xmlApply);
	}

	void DsGraph::showHistory(History& history, uint32_t seconds)
	{
		ZoneScoped;
		forgetHistory();
		m_points = static_cast<size_t>(seconds) * history.samplesPerSecond();
		setXRange({.min = -static_cast<int32_t>(seconds), .max = 0});
		setXCount(static_cast<int32_t>(m_points));

		m_history = &history;
		history.addListener(this);
		history.addSampleListener(this);
		addSeries(0, history.size());
	}

	void DsGraph::setYAxis(range_t range)
	{
		m_yAxis = range;
		applyYAxis();
	}

	void* DsGraph::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* graph = new DsGraph(attrOr(attrs, "name", "graph"), parentOf(state));
		deleteWith(graph->getRootPtr(), graph);
		return graph->getRootPtr();
	}

	void DsGraph::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& graph = static_cast<DsGraph&>(*LvObj::fromPtr(root));

		const char* yMin = lv_xml_get_value_of(attrs, "y_min");
		const char* yMax = lv_xml_get_value_of(attrs, "y_max");
		if (yMin || yMax)
		{
			graph.setYAxis({.min = yMin ? lv_xml_atoi(yMin) : graph.m_yAxis.min,
							.max = yMax ? lv_xml_atoi(yMax) : graph.m_yAxis.max});
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_y_max"))
		{
			lv_subject_add_observer_obj(subject, yMaxObserverCb, root, &graph);
		}

		const char* historyName = lv_xml_get_value_of(attrs, "history");
		if (historyName == nullptr)
		{
			return;
		}
		auto* history = dynamic_cast<History*>(Scope::findList(lv_xml_subject_ctx_get_active(), historyName));
		if (history == nullptr)
		{
			LOG_WARN("<ds_graph history=\"{:s}\">: no such history", historyName);
			return;
		}
		const int32_t seconds = lv_xml_atoi(attrOr(attrs, "seconds", "60"));
		graph.showHistory(*history, static_cast<uint32_t>(std::max(seconds, 1)));
	}

	void DsGraph::nameObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* graph = static_cast<DsGraph*>(lv_observer_get_user_data(observer));
		const auto& observers = graph->m_nameObservers;
		const auto it = std::find(observers.begin(), observers.end(), observer);
		// Not yet in the list while it is being added; addSeries sets the name itself
		if (it != observers.end())
		{
			graph->updateSeriesName(static_cast<size_t>(it - observers.begin()), lv_subject_get_string(subject));
		}
	}

	void DsGraph::yMaxObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* graph = static_cast<DsGraph*>(lv_observer_get_user_data(observer));
		graph->m_yLimit = lv_subject_get_int(subject);
		graph->applyYAxis();
	}

	void DsGraph::onListResized(size_t oldSize, size_t newSize)
	{
		if (newSize > oldSize)
		{
			addSeries(getSeriesCount(), newSize);
		}
		else
		{
			removeSeries(newSize);
		}
	}

	void DsGraph::onListDestroyed()
	{
		// The history is going: only let go of the rows, don't call back into it
		removeSeries(0);
		m_history = nullptr;
	}

	void DsGraph::onSampled()
	{
		ZoneScoped;
		for (size_t i = 0; i < m_history->size() && i < getSeriesCount(); i++)
		{
			const auto& values = m_history->row(i).values();
			if (!values.empty())
			{
				addData(i, values.back());
			}
		}
	}

	void DsGraph::addSeries(size_t from, size_t to)
	{
		ZoneScoped;
		for (size_t i = from; i < to; i++)
		{
			SeriesRow& row = m_history->row(i);
			if (!createSeries(lv_palette_main(static_cast<lv_palette_t>(i % LV_PALETTE_LAST)),
							  std::string(row.name.get())))
			{
				return;
			}
			const auto& values = row.values();
			const size_t skip = values.size() > m_points ? values.size() - m_points : 0;
			std::for_each(values.begin() + static_cast<std::ptrdiff_t>(skip),
						  values.end(),
						  [&](int32_t value) { addData(i, value); });
			m_nameObservers.push_back(lv_subject_add_observer(row.name.raw(), nameObserverCb, this));
		}
	}

	void DsGraph::removeSeries(size_t count)
	{
		while (m_nameObservers.size() > count)
		{
			lv_observer_remove(m_nameObservers.back());
			m_nameObservers.pop_back();
		}
		setSeriesCount(count);
	}

	void DsGraph::forgetHistory()
	{
		if (m_history == nullptr)
		{
			return;
		}
		removeSeries(0);
		m_history->removeListener(this);
		m_history->removeSampleListener(this);
		m_history = nullptr;
	}

	void DsGraph::applyYAxis()
	{
		setYRange({.min = m_yAxis.min, .max = std::max(m_yAxis.max, m_yLimit)});
	}
} // namespace UI::Xml
