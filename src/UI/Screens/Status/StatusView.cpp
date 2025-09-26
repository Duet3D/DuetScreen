#include "StatusView.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"

/**
 * @brief
 */
namespace UI
{
	StatusView::StatusView(LvObj& parent)
		: View("print_view", parent, layout_t(0, 0, 100, 100))
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().pad_zero);
		addStyle(Themes::getLvglStyles().pad_gap);
		addStyle(Themes::getLvglStyles().bg_dark);
		m_header.addStyle(Themes::getLvglStyles().card);
		m_printInfo.addStyle(Themes::getLvglStyles().card);
		m_footer.addStyle(Themes::getLvglStyles().card);

		// Layout
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_header.setWidth(LV_PCT(100));
		m_header.setHeight(LV_SIZE_CONTENT);
		// m_header.setFlexGrow(1);
		m_printInfo.setWidth(LV_PCT(100));
		m_printInfo.setFlexGrow(5);
		m_footer.setSize(LV_PCT(100), LV_PCT(20));

		// Header
		static const int32_t header_col_dsc[] = {LV_GRID_FR(5), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		static const int32_t header_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		m_header.setGridDsc(header_col_dsc, header_row_dsc);
		m_header.setGridCell(m_filename, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
		m_header.setGridCell(m_thumbnail, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
		m_header.setGridCell(m_progress, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 1, 1);

		m_progress.setRange(0, 100);

		// Footer
		m_footer.setFlexFlow(LV_FLEX_FLOW_ROW);

		m_pauseBtn.setText(_("pause"));
		m_resumeBtn.setText(_("resume"));
		m_printAgainBtn.setText(_("print_again"));
		m_cancelBtn.setText(_("cancel"));

		for (size_t i = 0; i < m_footer.getChildCnt(); i++)
		{
			lv_obj_t* child = m_footer.getChild(i);
			lv_obj_set_height(child, LV_PCT(100));
			lv_obj_set_style_min_height(child, LV_SIZE_CONTENT, 0);
			lv_obj_set_flex_grow(child, 1);
		}

		// Hide resume button initially
		m_resumeBtn.hide();

		// Cancel confirmation setup
		m_confirmCancel.setTitle(_("print_cancel_title"));
		m_confirmCancel.setText(_("print_cancel_message"));
		m_confirmCancel.okVisible(true);
		m_confirmCancel.cancelVisible(true);
		m_confirmCancel.hide();

		// Callbacks
		m_pauseBtn.addClickedCallback(onPauseClicked, this);
		m_resumeBtn.addClickedCallback(onResumeClicked, this);
		m_printAgainBtn.addClickedCallback(onPrintAgainClicked, this);
		m_cancelBtn.addClickedCallback(onCancelClicked, this);
	}

	bool StatusView::back()
	{
		UI_LOCK();
		return m_printInfo.back();
	}

	void StatusView::onPauseClicked(lv_event_t* e)
	{
		UI_LOCK();
		StatusView* view = static_cast<StatusView*>(lv_event_get_user_data(e));
		view->m_presenter->pausePrint();
	}

	void StatusView::onResumeClicked(lv_event_t* e)
	{
		UI_LOCK();
		StatusView* view = static_cast<StatusView*>(lv_event_get_user_data(e));
		view->m_presenter->resumePrint();
	}

	void StatusView::onPrintAgainClicked(lv_event_t* e)
	{
		UI_LOCK();
		StatusView* view = static_cast<StatusView*>(lv_event_get_user_data(e));
		view->m_presenter->printAgain();
	}

	void StatusView::onCancelClicked(lv_event_t* e)
	{
		UI_LOCK();
		StatusView* view = static_cast<StatusView*>(lv_event_get_user_data(e));
		view->m_confirmCancel.setOkCallback([view]() { view->m_presenter->cancelPrint(); });
		view->m_confirmCancel.show();
	}

	void StatusView::onShow()
	{
		m_printInfo.show();
	}

	void StatusView::onHide()
	{
		m_confirmCancel.hide();
	}

	void StatusView::setFilename(std::string_view filename)
	{
		UI_LOCK();
		LOG_DBG("'{:s}'", filename);
		m_filename.setText(filename);
	}

	void StatusView::updateProgress(uint32_t percent)
	{
		UI_LOCK();
		LOG_DBG("{:d}", percent);
		percent = std::min(percent, 100u);
		m_progress.setValue(percent);
		m_progressLabel.setText(fmt::format("{:d}%", percent));
	}

	void StatusView::updateExtrusionRate(float feedrate, float volumetric)
	{
		m_printInfo.updateExtrusionRate(feedrate, volumetric);
	}

	void StatusView::updateSpeed(float topSpeed, float requestedSpeed)
	{
		m_printInfo.updateSpeed(topSpeed, requestedSpeed);
	}

	void StatusView::updateFlowMultiplier(uint32_t multiplier)
	{
		m_printInfo.updateFlowMultiplier(multiplier);
	}

	void StatusView::updateSpeedMultiplier(uint32_t multiplier)
	{
		m_printInfo.updateSpeedMultiplier(multiplier);
	}

	void StatusView::updateAcceleration(uint32_t acceleration)
	{
		m_printInfo.updateAcceleration(acceleration);
	}

	void StatusView::updateZOffset(float offset)
	{
		m_printInfo.updateZOffset(offset);
	}

	void StatusView::updateLayerNumber(uint32_t layer)
	{
		m_printInfo.updateLayerNumber(layer);
	}

	void StatusView::updateElapsedTime(uint32_t elapsed)
	{
		m_printInfo.updateElapsedTime(elapsed);
	}

	void StatusView::updateRemainingTime(uint32_t remaining)
	{
		m_printInfo.updateRemainingTime(remaining);
	}

	void StatusView::updateLayer(float height, float maxHeight)
	{
		m_printInfo.updateLayer(height, maxHeight);
	}

	void StatusView::updateFanSpeed(uint32_t speed)
	{
		m_printInfo.updateFanSpeed(speed);
	}

	void StatusView::setThumbnail(const char* img)
	{
		UI_LOCK();
		LOG_DBG("'{:s}'", img);
		m_thumbnail.setSrc(img);
		m_header.updateLayout();
	}

	void StatusView::setPause(ControlVisibility visibility)
	{
		UI_LOCK();

		m_pauseBtn.setDisabled(visibility != ENABLED);
		if (visibility == HIDDEN)
		{
			m_pauseBtn.hide();
		}
		else
		{
			m_pauseBtn.show();
		}
	}

	void StatusView::setResume(ControlVisibility visibility)
	{
		UI_LOCK();

		m_resumeBtn.setDisabled(visibility != ENABLED);
		if (visibility == HIDDEN)
		{
			m_resumeBtn.hide();
		}
		else
		{
			m_resumeBtn.show();
		}
	}

	void StatusView::setPrintAgain(ControlVisibility visibility)
	{
		UI_LOCK();

		m_printAgainBtn.setDisabled(visibility != ENABLED);
		if (visibility == HIDDEN)
		{
			m_printAgainBtn.hide();
		}
		else
		{
			m_printAgainBtn.show();
		}
	}

	void StatusView::setCancel(ControlVisibility visibility)
	{
		UI_LOCK();

		m_cancelBtn.setDisabled(visibility != ENABLED);
		if (visibility == HIDDEN)
		{
			m_cancelBtn.hide();
		}
		else
		{
			m_cancelBtn.show();
		}
	}
} // namespace UI
