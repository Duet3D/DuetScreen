#include "StatusView.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"

/**
 * @brief
 */
namespace UI
{
	StatusView::StatusView(const std::string& name, LvObj& parent)
		: View(name, parent, layout_t(0, 0, 100, 100))
	{
		ZoneScoped;
		UI_LOCK();

		addStyle(Themes::getLvglStyles().pad_zero);
		addStyle(Themes::getLvglStyles().pad_gap);
		addStyle(Themes::getLvglStyles().bg_dark);
		m_header.addStyle(Themes::getLvglStyles().card);
		m_printInfo.addStyle(Themes::getLvglStyles().card);
		m_footer.addStyle(Themes::getLvglStyles().card);

		m_printAgainBtn.addStyle(Themes::getLvglStyles().actionBtn);
		m_pauseBtn.addStyle(Themes::getLvglStyles().actionBtn);
		m_resumeBtn.addStyle(Themes::getLvglStyles().actionBtn);

		// Layout
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_header.setWidth(LV_PCT(100));
		m_header.setHeight(LV_SIZE_CONTENT);
		m_header.setMaxHeight(LV_PCT(30));
		// m_header.setFlexGrow(1);
		m_printInfo.setWidth(LV_PCT(100));
		m_printInfo.setFlexGrow(5);
		m_footer.setSize(LV_PCT(100), LV_PCT(15));
		m_footer.setMinHeight(LV_SIZE_CONTENT);

		// Header
		static const int32_t header_col_dsc[] = {LV_GRID_FR(5), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		static const int32_t header_row_dsc[] = {
			LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		m_header.setGridDsc(header_col_dsc, header_row_dsc);
		m_header.setGridCell(m_printFinishedLabel, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0, 1);
		m_header.setGridCell(m_filename, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 1, 1);
		m_header.setGridCell(m_thumbnail, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_START, 0, 4);
		m_header.setGridCell(m_progress, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_END, 3, 1);

		m_printFinishedLabel.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_printFinishedLabel.setText(_("status.print_finished"));
		m_filename.setWidth(LV_PCT(100));
		m_filename.setHeight(LV_SIZE_CONTENT);
		// m_filename.setLongMode(LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
		m_thumbnail.setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);
		m_thumbnail.setHeight(LV_PCT(100));
		m_thumbnail.setMinHeight(50);
		m_thumbnail.setMaxWidth(LV_PCT(30));
		m_header.addEventCallback(
			[this](lv_event_t*)
			{
				[[unlikely]] if (m_thumbnail.getRootPtr() == nullptr)
				{
					/* Check if the thumbnail has been deleted */
					return;
				}
				/**
				 * This will effectively override the thumbnail grid layout so that it will always be the largest
				 * rectangle with the same aspect ratio as the source image that can fit in the header without
				 * increasing the header height.
				 *
				 * Need to use the header content height instead of the thumbnail height because the thumbnail height
				 * won't have been updated yet.
				 */
				const void* src = m_thumbnail.getSrc();
				if (src == nullptr)
				{
					m_thumbnail.setWidth(0);
					return;
				}

				const int32_t src_width = m_thumbnail.getSrcWidth();
				const int32_t src_height = m_thumbnail.getSrcHeight();
				if (src_width <= 0 || src_height <= 0)
				{
					m_thumbnail.setWidth(0);
					return;
				}

				const int32_t aspect_ratio_100 = 100 * src_width / src_height;
				const int32_t height = lv_obj_calc_content_height(m_header.getRootPtr());
				const int32_t width = height * aspect_ratio_100 / 100;
				LOG_DBG("Setting thumbnail width from {:d} to {:d} based on height", m_thumbnail.getWidth(), width);
				m_thumbnail.setWidth(width);
			},
			LV_EVENT_CHILD_CHANGED);
		m_thumbnail.setWidth(50);
		m_progress.setHeight(LV_SIZE_CONTENT);
		m_progress.setRange(0, 100);
		m_progress.setLabelFormat("{}%");

		// Footer
		m_footer.setFlexFlow(LV_FLEX_FLOW_ROW);

		m_pauseBtn.setText(_("status.pause"));
		m_resumeBtn.setText(_("status.resume"));
		m_printAgainBtn.setText(_("status.print_again"));
		m_cancelBtn.setText(_("status.cancel"));

		m_pauseBtn.setIcon("pause.png");
		m_resumeBtn.setIcon("resume.png");
		m_printAgainBtn.setIcon("print_again.png");
		m_cancelBtn.setIcon("cancel.png");

		m_footer.iterateChildren(
			[](size_t /* i */, LvObj& child)
			{
				child.setHeight(LV_PCT(100));
				child.setMinHeight(LV_SIZE_CONTENT);
				child.setFlexGrow(1);
			});

		// Hide resume button initially
		m_resumeBtn.hide();

		// Cancel confirmation setup
		m_confirmCancel.setTitle(_("status.print_cancel_title"));
		m_confirmCancel.setText(_("status.print_cancel_message"));
		m_confirmCancel.okVisible(true);
		m_confirmCancel.cancelVisible(true);
		m_confirmCancel.setOkCallback([this]() { m_presenter->cancelPrint(); });

		// Callbacks
		m_pauseBtn.addClickedCallback(onPauseClicked, this);
		m_resumeBtn.addClickedCallback(onResumeClicked, this);
		m_printAgainBtn.addClickedCallback(onPrintAgainClicked, this);
		m_cancelBtn.addClickedCallback(onCancelClicked, this);
	}

	bool StatusView::back()
	{
		ZoneScoped;
		UI_LOCK();
		return m_printInfo.back();
	}

	void StatusView::onPauseClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		StatusView* view = static_cast<StatusView*>(lv_event_get_user_data(e));
		view->m_presenter->pausePrint();
	}

	void StatusView::onResumeClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		StatusView* view = static_cast<StatusView*>(lv_event_get_user_data(e));
		view->m_presenter->resumePrint();
	}

	void StatusView::onPrintAgainClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		StatusView* view = static_cast<StatusView*>(lv_event_get_user_data(e));
		view->m_presenter->printAgain();
	}

	void StatusView::onCancelClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		StatusView* view = static_cast<StatusView*>(lv_event_get_user_data(e));
		openModal(&view->m_confirmCancel);
	}

	void StatusView::onInit()
	{
		ZoneScoped;
		if (LvObj* host = getModalHost())
		{
			m_confirmCancel.setParent(*host);
		}
	}

	void StatusView::onShow() {}

	void StatusView::onHide() {}

	void StatusView::setFilename(std::string_view filename, bool isInProgress)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("'{:s}'", filename);
		m_filename.setText(filename);
		m_printFinishedLabel.setVisible(!isInProgress);
	}

	void StatusView::updateProgress(uint32_t percent)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("{:d}", percent);
		m_progress.setValue(percent);
	}

	void StatusView::setThumbnail(const char* img)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("'{:s}'", img ? img : "null");
		m_thumbnail.setSrc(img);
		m_header.updateLayout();
	}

	void StatusView::setPause(ControlVisibility visibility)
	{
		ZoneScoped;
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
		ZoneScoped;
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
		ZoneScoped;
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
		ZoneScoped;
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

	void StatusView::setNumberPad(ModalNumberPad* np)
	{
		ZoneScoped;
		m_printInfo.setNumberPad(np);
	}
} // namespace UI
