/*
 * PrintInfo.h
 *
 *  Created on: 2025-02-27
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/List/List.h"
#include "UI/Core/View.h"
#include "UI/Widgets/BabyStep/BabyStep.h"

namespace UI
{
	class PrintInfo : public LvContainer
	{
	  public:
		PrintInfo(const std::string& name, LvObj& parent);

		void setAxisCount(size_t count);
		void setPosition(size_t index, char axis_letter, float value);
		void updateExtrusionRate(float feedrate, float volumetric);
		void updateSpeed(float topSpeed, float requestedSpeed);
		void updateFlowMultiplier(uint32_t multiplier);
		void updateSpeedMultiplier(uint32_t multiplier);
		void updateElapsedTime(uint32_t elapsed);
		void updateRemainingTime(uint32_t remaining);
		void updateLayer(float height, float maxHeight);
		void updateFanSpeed(uint32_t speed);
		void updateAcceleration(uint32_t acceleration);
		void updatePosition(float x, float y, float z);
		void updateZOffset(float offset);
		void updateLayerNumber(uint32_t layer);

		virtual bool back() override;

	  private:
		class SpeedInfo : public LvObj
		{
		  public:
			SpeedInfo(const std::string& name, LvObj& parent);

			void updateSpeed(float topSpeed, float requestedSpeed);
			void updateSpeedMultiplier(uint32_t multiplier);
			void updateAcceleration(uint32_t acceleration);
			void updateZOffset(float offset);
			void updatePrintHeight(float height);
			void updateLayerNumber(uint32_t layer);

		  private:
			LvLabel m_speed{"speed", getRoot()};
			LvLabel m_speedMultiplier{"speed_multiplier", getRoot()};
			LvLabel m_acceleration{"acceleration", getRoot()};
			LvLabel m_z_offset{"z_offset", getRoot()};
			LvLabel m_z_height{"z_height", getRoot()};
			LvLabel m_layer{"layer", getRoot()};
		};

		static void openSubView(lv_event_t* e);
		void onShow() override;
		void onHide() override;

		List<Button> m_positions{"positions", getRoot()};

		LvContainer m_speedCont{"speed_cont", getRoot()};
		LvLabel m_speedHeader{"speed_header", m_speedCont};
		LvLabel m_currentSpeed{"current_speed", m_speedCont};
		LvLabel m_requestedSpeed{"top_speed", m_speedCont};
		Button m_speedMultiplier{"speed_multiplier", m_speedCont};

		LvContainer m_flowCont{"flow_cont", getRoot()};
		LvLabel m_flowHeader{"flow_header", m_flowCont};
		LvLabel m_extruderFeedrate{"extruder_feedrate", m_flowCont};
		LvLabel m_flowRate{"flow_rate", m_flowCont};
		Button m_flowMultiplier{"flow_multiplier", m_flowCont};

		LvContainer m_timeCont{"time_cont", getRoot()};
		LvLabel m_elapsedTime{"elapsed_time", m_timeCont};
		LvLabel m_remainingTime{"remaining_time", m_timeCont};

		BabyStep m_babyStep{"baby_step", getRoot()};

		SpeedInfo m_speedInfo{"speed_info", getRoot()};

		bool m_initialised = false;
	};
} // namespace UI
