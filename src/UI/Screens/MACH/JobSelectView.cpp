/*
 * JobSelectView.cpp
 *
 *  Created on: 2026-04-07
 *      Author: Andy Everitt
 */

#include "JobSelectView.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_set>

namespace UI
{
	static lv_color_t GetHistoryColor(const size_t index)
	{
		static constexpr lv_palette_t palette[] = {
			LV_PALETTE_BLUE,
			LV_PALETTE_RED,
			LV_PALETTE_GREEN,
			LV_PALETTE_ORANGE,
			LV_PALETTE_PURPLE,
			LV_PALETTE_CYAN,
			LV_PALETTE_TEAL,
			LV_PALETTE_DEEP_PURPLE,
			LV_PALETTE_PINK,
			LV_PALETTE_AMBER,
		};
		return lv_palette_main(palette[index % std::size(palette)]);
	}

	JobSelectView::JobSelectView(const std::string& name, LvObj& parent)
		: View(name, parent, layout_t{0, 0, 100, 100})
	{
		setGridDsc({LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST},
				   {LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST});

		setGridCell(m_currentJobs, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
		setGridCell(m_nextJob, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		setGridCell(m_jobHistory, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
	}

	JobSelectView::CurrentJobs::CurrentJobs(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

		for (auto& job : m_motionSystemJobs)
		{
			job.setWidth(LV_PCT(100));
			job.setFlexGrow(1);
			job.setJobName("Job Name");
		}

		m_motionSystemJobs[0].setHeader("Motion System Job 1");
		m_motionSystemJobs[1].setHeader("Motion System Job 2");
	}

	JobSelectView::CurrentJobs::MotionSystemJob::MotionSystemJob(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		setGridDsc({LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST},
				   {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST});

		setGridCell(m_header, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
		setGridCell(m_jobName, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 1, 1);
		setGridCell(m_thumbnail, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 2);
	}

	JobSelectView::NextJob::NextJob(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		m_nextJobsList.setSize(LV_PCT(100), LV_PCT(100));
		m_nextJobsList.setTitle("Scheduled Next Job");

		m_nextJobsList.getListContainer().setSize(LV_PCT(100), LV_PCT(100));
		m_nextJobsList.getListContainer().setFlexGrow(1);
		m_nextJobsList.getListContainer().setFlexAlign(
			LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY);
		m_nextJobsList.getListContainer().setFlexFlow(LV_FLEX_FLOW_COLUMN_WRAP);
	}

	JobSelectView::JobHistory::JobHistory(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		m_graph.setSize(LV_PCT(100), LV_PCT(100));
		m_graph.addStyle(Themes::getLvglStyles().card);
		m_graph.showLegend(true);
		m_graph.setXRange({.min = 0, .max = MAX_SAMPLES - 1});
		m_graph.setYRange({.min = 0, .max = 1});
		m_graph.setXCount(MAX_SAMPLES);
		m_graph.showXRange(false);
	}

	void JobSelectView::setJobs(const std::vector<std::optional<std::string>>& jobs)
	{
		m_nextJob.setJobCount(jobs.size());
		for (size_t i = 0; i < jobs.size(); ++i)
		{
			if (jobs[i].has_value())
			{
				m_nextJob.setJobName(i, *jobs[i]);
			}
			else
			{
				m_nextJob.setJobName(i, "null");
			}
		}
	}

	void JobSelectView::setCurrentJob(size_t index, std::string_view jobName)
	{
		if (index < m_currentJobs.m_motionSystemJobs.size())
		{
			m_currentJobs.m_motionSystemJobs[index].setJobName(jobName);
		}
	}

	void JobSelectView::setCurrentJobThumbnail(size_t index, const void* thumbnailSrc)
	{
		if (index < m_currentJobs.m_motionSystemJobs.size())
		{
			m_currentJobs.m_motionSystemJobs[index].setThumbnail(thumbnailSrc);
		}
	}

	void JobSelectView::setNextJob(std::string_view jobName)
	{
		m_nextJob.m_nextJobsList.iterateListItems([&](size_t /* index */, Button& btn)
												  { btn.setChecked(btn.getText() == jobName); });
	}

	void JobSelectView::updateJobHistory(const std::vector<OM::MACH::JobHistoryEntry>& history)
	{
		auto& graph = m_jobHistory.m_graph;
		auto& seriesByName = m_jobHistory.m_seriesByName;

		std::unordered_set<std::string> presentNames;
		presentNames.reserve(history.size());
		for (const auto& entry : history)
		{
			presentNames.insert(entry.name);
		}

		std::vector<std::pair<std::string, size_t>> toRemove;
		toRemove.reserve(seriesByName.size());
		for (const auto& [name, index] : seriesByName)
		{
			if (!presentNames.contains(name))
			{
				toRemove.emplace_back(name, index);
			}
		}

		std::sort(toRemove.begin(), toRemove.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

		for (const auto& [name, index] : toRemove)
		{
			graph.clear(index);
			seriesByName.erase(name);

			for (auto& [existingName, existingIndex] : seriesByName)
			{
				UNUSED(existingName);
				if (existingIndex > index)
				{
					existingIndex -= 1;
				}
			}
		}

		for (const auto& entry : history)
		{
			auto indexIt = seriesByName.find(entry.name);
			if (indexIt == seriesByName.end())
			{
				const size_t newIndex = graph.getSeriesCount();
				if (!graph.createSeries(GetHistoryColor(newIndex), entry.name))
				{
					LOG_WARN("Failed to create history series for '{}'", entry.name);
					continue;
				}
				seriesByName.emplace(entry.name, newIndex);

				// Backfill old samples so new series aligns with existing timeline.
				const size_t backfillCount =
					std::min(m_jobHistory.m_sampleCount, static_cast<size_t>(JobHistory::MAX_SAMPLES));
				for (size_t i = 0; i < backfillCount; ++i)
				{
					graph.addData(newIndex, 0);
				}

				indexIt = seriesByName.find(entry.name);
			}

			graph.addData(indexIt->second, entry.count);
		}

		m_jobHistory.m_sampleCount += 1;

		if (seriesByName.empty())
		{
			graph.setYRange({.min = 0, .max = 1});
			return;
		}

		int32_t minCount = std::numeric_limits<int32_t>::max();
		int32_t maxCount = std::numeric_limits<int32_t>::min();
		for (const auto& [name, index] : seriesByName)
		{
			UNUSED(name);
			for (const int32_t value : graph.getSeriesYArray(index))
			{
				if (value == LV_CHART_POINT_NONE)
				{
					continue;
				}
				minCount = std::min(minCount, value);
				maxCount = std::max(maxCount, value);
			}
		}

		if (minCount == std::numeric_limits<int32_t>::max() || maxCount == std::numeric_limits<int32_t>::min())
		{
			graph.setYRange({.min = 0, .max = 1});
			return;
		}

		if (minCount == maxCount)
		{
			maxCount += 1;
		}

		graph.setYRange({.min = minCount, .max = maxCount});
	}

	void JobSelectView::NextJob::setJobCount(size_t count)
	{
		m_nextJobsList.setItemCount(count,
									[](size_t index, LvObj& parent)
									{
										auto btn = std::make_unique<Button>(fmt::format("jobButton{}", index), parent);
										btn->setText(fmt::format("Job {}", index + 1));
										btn->setSize(LV_PCT(30), LV_PCT(45));
										return btn;
									});
	}

	void JobSelectView::NextJob::setJobName(size_t index, std::string_view jobName)
	{
		if (auto btn = m_nextJobsList.getItem(index))
		{
			btn->setText(jobName);
		}
	}
} // namespace UI
