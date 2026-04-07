/*
 * JobSelectView.h
 *
 *  Created on: 2026-04-07
 *      Author: Andy Everitt
 */

#pragma once

#include "JobSelectPresenter.h"
#include "ObjectModel/MACH.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Graph/Graph.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvImage.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/List/List.h"
#include "UI/Core/View.h"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace UI
{
	class JobSelectView : public View<JobSelectPresenter>
	{
	  public:
		class CurrentJobs : public LvContainer
		{
		  public:
			class MotionSystemJob : public LvContainer
			{
			  public:
				MotionSystemJob(const std::string& name, LvObj& parent);

                void setHeader(std::string_view header) { m_header.setText(header); }
                void setJobName(std::string_view jobName) { m_jobName.setText(jobName); }
                void setThumbnail(const void* src) { m_thumbnail.setSrc(src); }

				LvLabel m_header{"header", getRoot()};
				LvLabel m_jobName{"jobName", getRoot()};
				LvImage m_thumbnail{"thumbnail", getRoot()};
			};

			CurrentJobs(const std::string& name, LvObj& parent);

			std::array<MotionSystemJob, 2> m_motionSystemJobs{
                MotionSystemJob{"motionSystemJob1", getRoot()},
                MotionSystemJob{"motionSystemJob2", getRoot()},
            };
		};

		class NextJob : public LvContainer
		{
		  public:
			NextJob(const std::string& name, LvObj& parent);

			void setJobCount(size_t count);
			void setJobName(size_t index, std::string_view jobName);
			void setJobSelectedCallback(std::function<void(size_t index, std::string_view jobName)> callback)
			{
				m_jobSelectedCallback = std::move(callback);
			}

			List<Button> m_nextJobsList{"jobList", getRoot()};

		  private:
			std::function<void(size_t index, std::string_view jobName)> m_jobSelectedCallback;
		};

		class JobHistory : public LvContainer
		{
		  public:
			JobHistory(const std::string& name, LvObj& parent);

			LvLabel m_header{"header", getRoot()};
			Graph m_graph{"chart", getRoot()};

			std::unordered_map<std::string, size_t> m_seriesByName;
			size_t m_sampleCount = 0;
			static constexpr int32_t MAX_SAMPLES = 120;
		};

		JobSelectView(const std::string& name, LvObj& parent);

		void setJobs(const std::vector<std::optional<std::string>>& jobs);
		void setCurrentJob(size_t index, std::string_view jobName);
		void setCurrentJobThumbnail(size_t index, const void* thumbnailSrc);
		void setNextJob(std::string_view jobName);
		void updateJobHistory(const std::vector<OM::MACH::JobHistoryEntry>& history);

	  private:
		CurrentJobs m_currentJobs{"currentJobs", getRoot()};
		NextJob m_nextJob{"nextJob", getRoot()};
		JobHistory m_jobHistory{"jobHistory", getRoot()};
	};
} // namespace UI
