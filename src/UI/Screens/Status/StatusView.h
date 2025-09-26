#pragma once

#include "FineTune.h"
#include "PrintInfo.h"
#include "StatusPresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Modal/Modal.h"
#include "UI/Core/View.h"

namespace UI
{
	class StatusView : public View<StatusPresenter>
	{
	  public:
		enum ControlVisibility
		{
			ENABLED,
			DISABLED,
			HIDDEN
		};

		StatusView(LvObj& parent);

		void setFilename(std::string_view filename);
		void updateProgress(uint32_t percent);

		void setAxisCount(size_t count) { m_printInfo.setAxisCount(count); }
		void setPosition(size_t index, char axis_letter, float value)
		{
			m_printInfo.setPosition(index, axis_letter, value);
		}
		void updateExtrusionRate(float feedrate, float volumetric);
		void updateSpeed(float topSpeed, float requestedSpeed);
		void updateFlowMultiplier(uint32_t multiplier);
		void updateSpeedMultiplier(uint32_t multiplier);
		void updateAcceleration(uint32_t acceleration);
		void updateZOffset(float offset);
		void updateLayerNumber(uint32_t layer);
		void updateElapsedTime(uint32_t elapsed);
		void updateRemainingTime(uint32_t remaining);
		void updateLayer(float height, float maxHeight);
		void updateFanSpeed(uint32_t speed);
		void setThumbnail(const char* img);

		void setPause(ControlVisibility visibility);
		void setResume(ControlVisibility visibility);
		void setPrintAgain(ControlVisibility visibility);
		void setCancel(ControlVisibility visibility);

		virtual bool back() override;

	  private:
		void onShow() override;
		void onHide() override;

		static void onPauseClicked(lv_event_t* e);
		static void onResumeClicked(lv_event_t* e);
		static void onPrintAgainClicked(lv_event_t* e);
		static void onCancelClicked(lv_event_t* e);

		// Status panels
		LvContainer m_header{"header", getRoot()};
		PrintInfo m_printInfo{"print_info", getRoot()};
		LvContainer m_footer{"footer", getRoot()};

		// Header
		LvBar m_progress{"progress", m_header};
		LvLabel m_progressLabel{"progress_label", m_progress};
		LvLabel m_filename{"filename", m_header};
		LvImage m_thumbnail{"thumbnail", m_header};

		// Control buttons
		Button m_pauseBtn{"pause", m_footer};
		Button m_resumeBtn{"resume", m_footer};
		Button m_printAgainBtn{"print_again", m_footer};
		Button m_cancelBtn{"cancel", m_footer};

		Modal<MessageBox> m_confirmCancel{"start_print", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT)};
	};
} // namespace UI
