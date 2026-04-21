/*
 * ObjectCancelView.cpp
 *
 *  Created on: 2025-03-18
 *      Author: Andy Everitt
 */

#include "ObjectCancelView.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"

namespace UI
{
	ObjectItem::ObjectItem(size_t index, LvObj& parent, ObjectCancelView& view)
		: ListItem(index, parent)
		, m_view(view)
		, m_label("label", getRoot())
	{
		ZoneScoped;
		UI_LOCK();

		setSize(LV_PCT(100), LV_SIZE_CONTENT);
		setFlag(LV_OBJ_FLAG_CLICKABLE, true);
		m_label.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		addEventCallback(
			[](lv_event_t* event)
			{
				ZoneScopedN("ObjectItem::click");
				UI_LOCK();
				auto* item = static_cast<ObjectItem*>(lv_event_get_user_data(event));
				if (item == nullptr)
				{
					LOG_WARN("ObjectItem is null on click");
					return;
				}
				if (item->m_view.getPresenter() != nullptr)
				{
					item->m_view.getPresenter()->onObjectSelected(item->getIndex());
				}
			},
			LV_EVENT_CLICKED,
			this);
	}

	void ObjectItem::update(std::string_view name, bool isCurrent, bool isCancelled)
	{
		ZoneScoped;
		UI_LOCK();

		m_label.setText(fmt::format("[{:d}]: {:s}", getIndex(), name));

		{
			const auto& cancelledStyle = Themes::getComponentStyles().object_cancel_cancelled;
			if (isCancelled)
			{
				if (!hasStyle(cancelledStyle))
				{
					addStyle(cancelledStyle);
				}
			}
			else
			{
				removeStyle(cancelledStyle);
			}
		}

		{
			const auto& currentStyle = Themes::getComponentStyles().object_cancel_current;
			if (isCurrent)
			{
				if (!hasStyle(currentStyle))
				{
					addStyle(currentStyle);
				}
			}
			else
			{
				removeStyle(currentStyle);
			}
		}
	}

	ObjectCancelView::ObjectCancelView(const std::string& name, LvObj& parent)
		: View(name, parent, layout_t(0, 0, 100, 100))
	{
		ZoneScoped;
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);
		m_canvas.addStyle(Themes::getLvglStyles().card);
		m_canvas.addStyle(Themes::getLvglStyles().pad_base);
		m_canvas.setStylePad(20, LV_PART_MAIN, Padding::RIGHT);
		m_objectList.addStyle(Themes::getLvglStyles().card);
		m_cancelCurrentBtn.addStyle(Themes::getLvglStyles().actionBtn);

		setGridDsc(m_layoutColDsc, m_layoutRowDsc);
		setGridCell(m_canvas, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
		setGridCell(m_objectList, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		setGridCell(m_cancelCurrentBtn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 1, 1);

		auto& header = m_canvas.getHeader();
		header.setFlexFlow(LV_FLEX_FLOW_ROW);
		header.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_canvas.getTitle().addStyle(Themes::getLvglStyles().text_emphasis);
		m_canvas.setTitle(_("object_cancel.current_object_prefix"));
		m_currentObjectLabel.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_currentObjectLabel.setText(_("object_cancel.current_object_none"));

		m_canvas.setResolution(200, 200);
		m_canvas.showTitle(true);
		m_canvas.drawGrid();

		m_canvas.addEventCallback(
			[this](lv_event_t*)
			{
				ZoneScopedN("ObjectCancelView::canvas_style_changed");
				m_canvas.clear();
				m_canvas.drawGrid();
			},
			LV_EVENT_STYLE_CHANGED);

		m_objectList.setTitle(_("object_cancel.objects_header"));
		m_objectList.setListGrow(1);

		m_cancelCurrentBtn.setText(_("object_cancel.cancel_current"));
		m_cancelCurrentBtn.setSize(LV_PCT(100), 50);
		m_cancelCurrentBtn.setMinHeight(LV_SIZE_CONTENT);
		m_cancelCurrentBtn.addClickedCallback(onCancelCurrentClicked, this);
		m_overlayRects.reserve(MAX_TRACKED_OBJECTS);

		m_confirmModal.okVisible(true);
		m_confirmModal.cancelVisible(true);
		m_confirmModal.getOkBtn().addStyle(Themes::getLvglStyles().actionBtn);
	}

	ObjectCancelView::~ObjectCancelView() = default;

	void ObjectCancelView::setAxisRange(Canvas::range_t xRange, Canvas::range_t yRange)
	{
		ZoneScoped;
		UI_LOCK();
		bool rangeChanged = false;
		const auto& currentXRange = m_canvas.getXRange();
		const auto& currentYRange = m_canvas.getYRange();
		if (currentXRange.min != xRange.min || currentXRange.max != xRange.max)
		{
			rangeChanged = true;
			m_canvas.setXRange(xRange);
		}
		if (currentYRange.min != yRange.min || currentYRange.max != yRange.max)
		{
			rangeChanged = true;
			m_canvas.setYRange(yRange);
		}
		if (rangeChanged)
		{
			m_canvas.clear();
			m_canvas.drawGrid();
		}
	}

	void ObjectCancelView::beginObjectOverlayUpdate()
	{
		ZoneScoped;
		UI_LOCK();
		for (auto& overlay : m_overlayRects)
		{
			if (overlay != nullptr && overlay->rect != nullptr)
			{
				overlay->rect->hide();
			}
		}
	}

	ObjectCancelView::OverlayRect* ObjectCancelView::ensureOverlayRect(size_t index)
	{
		ZoneScoped;
		UI_LOCK();

		if (index >= m_overlayRects.size())
		{
			m_overlayRects.resize(index + 1);
		}

		auto& overlay = m_overlayRects[index];
		if (overlay == nullptr)
		{
			overlay = std::make_unique<OverlayRect>();
			overlay->index = index;
			overlay->owner = this;
			overlay->rect = std::make_unique<LvContainer>(fmt::format("obj_overlay_{:d}", index), m_canvas.getCanvas());

			overlay->rect->setStylePad(0);
			overlay->rect->setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
			overlay->rect->setFlag(LV_OBJ_FLAG_CLICKABLE, true);
			overlay->rect->setFlag(LV_OBJ_FLAG_FLOATING, true);
			overlay->rect->addStyle(Themes::getComponentStyles().object_cancel_base);
			overlay->rect->addEventCallback(onObjectOverlayPressed, LV_EVENT_PRESSED, overlay.get());
			lv_obj_set_style_radius(overlay->rect->getRootPtr(), 2, LV_PART_MAIN);
			lv_obj_set_style_border_width(overlay->rect->getRootPtr(), 1, LV_PART_MAIN);
			lv_obj_set_style_border_color(overlay->rect->getRootPtr(), lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
		}

		return overlay.get();
	}

	void ObjectCancelView::updateObjectOverlay(size_t index,
											   const OM::JobObject::Point& bounds,
											   bool isCurrent,
											   bool isCancelled)
	{
		ZoneScoped;
		UI_LOCK();
		auto* overlay = ensureOverlayRect(index);
		if (overlay == nullptr || overlay->rect == nullptr)
		{
			return;
		}

		int32_t px1, py1, px2, py2;
		if (!m_canvas.posToPx(static_cast<float>(bounds.x[0]), static_cast<float>(bounds.y[0]), px1, py1))
		{
			LOG_WARN("ObjectCancel: object {:d} lower-left out of axis range", index);
			return;
		}
		if (!m_canvas.posToPx(static_cast<float>(bounds.x[1]), static_cast<float>(bounds.y[1]), px2, py2))
		{
			LOG_WARN("ObjectCancel: object {:d} upper-right out of axis range", index);
			return;
		}

		const int32_t x = std::min(px1, px2);
		const int32_t y = std::min(py1, py2);
		const int32_t w = std::max(1, std::abs(px2 - px1));
		const int32_t h = std::max(1, std::abs(py2 - py1));

		uint32_t bufW = 0;
		uint32_t bufH = 0;
		if (!m_canvas.getResolution(bufW, bufH) || bufW == 0 || bufH == 0)
		{
			return;
		}

		auto& canvasObj = m_canvas.getCanvas();
		canvasObj.updateLayout();
		const int32_t widgetW = canvasObj.getWidth();
		const int32_t widgetH = canvasObj.getHeight();
		if (widgetW <= 0 || widgetH <= 0)
		{
			return;
		}

		const int32_t xScaled = static_cast<int32_t>((static_cast<int64_t>(x) * widgetW) / bufW);
		const int32_t yScaled = static_cast<int32_t>((static_cast<int64_t>(y) * widgetH) / bufH);
		const int32_t wScaled = std::max(1, static_cast<int32_t>((static_cast<int64_t>(w) * widgetW) / bufW));
		const int32_t hScaled = std::max(1, static_cast<int32_t>((static_cast<int64_t>(h) * widgetH) / bufH));

		overlay->rect->setPos(xScaled, yScaled);
		overlay->rect->setSize(wScaled, hScaled);

		{
			const auto& cancelledStyle = Themes::getComponentStyles().object_cancel_cancelled;
			if (isCancelled)
			{
				if (!overlay->rect->hasStyle(cancelledStyle))
				{
					overlay->rect->addStyle(cancelledStyle);
				}
			}
			else
			{
				overlay->rect->removeStyle(cancelledStyle);
			}
		}

		{
			const auto& currentStyle = Themes::getComponentStyles().object_cancel_current;
			if (isCurrent)
			{
				if (!overlay->rect->hasStyle(currentStyle))
				{
					overlay->rect->addStyle(currentStyle);
				}
			}
			else
			{
				overlay->rect->removeStyle(currentStyle);
			}
		}
		overlay->rect->show(true);
	}

	void ObjectCancelView::setObjectCount(size_t count)
	{
		ZoneScoped;
		UI_LOCK();
		m_objectList.setItemCount(count, *this);
	}

	void ObjectCancelView::setCurrentObjectName(std::string_view name)
	{
		ZoneScoped;
		UI_LOCK();
		const std::string_view shownName = name.empty() ? _("object_cancel.current_object_none") : name;
		m_currentObjectLabel.setText(shownName);
	}

	void ObjectCancelView::updateObjectItem(size_t index, std::string_view name, bool isCurrent, bool isCancelled)
	{
		ZoneScoped;
		UI_LOCK();
		auto* item = m_objectList.getItem(index);
		if (item == nullptr)
		{
			LOG_WARN("ObjectCancel: list item {:d} not found", index);
			return;
		}
		item->update(name, isCurrent, isCancelled);
	}

	void ObjectCancelView::showConfirmModal(size_t index, std::string_view name, bool isCancelled)
	{
		ZoneScoped;
		UI_LOCK();
		if (isCancelled)
		{
			m_confirmModal.setTitle(_("object_cancel.confirm_uncancel_title"));
			m_confirmModal.setText(_("object_cancel.confirm_uncancel_text", name));
		}
		else
		{
			m_confirmModal.setTitle(_("object_cancel.confirm_cancel_title"));
			m_confirmModal.setText(_("object_cancel.confirm_cancel_text", name));
		}
		const bool newActiveState = isCancelled;
		m_confirmModal.setOkCallback(
			[this, index, newActiveState]()
			{
				if (m_presenter != nullptr)
				{
					m_presenter->setJobObjectActive(index, newActiveState);
				}
			});
		openModal(&m_confirmModal);
	}

	void ObjectCancelView::setCancelCurrentEnabled(bool enabled)
	{
		ZoneScoped;
		UI_LOCK();
		m_cancelCurrentBtn.setState(LV_STATE_DISABLED, !enabled);
	}

	void ObjectCancelView::onInit()
	{
		ZoneScoped;
		m_confirmModal.setParent(HomeView::instance().getMainWindow());
	}

	void ObjectCancelView::onShow()
	{
		ZoneScoped;
		UI_LOCK();

		const auto& actionBtn = Themes::getLvglStyles().actionBtn;
		const bool hasActionBtnStyle = m_cancelCurrentBtn.hasStyle(actionBtn);
		const bool shouldHaveActionBtnStyle = !StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS);
		if (hasActionBtnStyle == shouldHaveActionBtnStyle)
			return;

		/* Add/remove actionBtn style from cancel current button */
		if (shouldHaveActionBtnStyle)
		{
			m_cancelCurrentBtn.addStyle(actionBtn);
		}
		else
		{
			m_cancelCurrentBtn.removeStyle(actionBtn);
		}
	}

	void ObjectCancelView::onCancelCurrentClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		auto* view = static_cast<ObjectCancelView*>(lv_event_get_user_data(e));
		if (view == nullptr || view->m_presenter == nullptr)
		{
			return;
		}

		if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
		{
			view->m_presenter->cancelCurrentObject();
			return;
		}

		const int8_t currentIndex = OM::GetCurrentJobObjectIndex();
		auto currentObj = currentIndex >= 0 ? OM::GetJobObject(static_cast<size_t>(currentIndex)) : nullptr;
		const std::string_view currentName = (currentObj != nullptr && !currentObj->name.empty())
												 ? std::string_view(currentObj->name)
												 : std::string_view(_("object_cancel.current_object_none"));

		view->m_confirmModal.setTitle(_("object_cancel.confirm_cancel_current_title"));
		view->m_confirmModal.setText(_("object_cancel.confirm_cancel_current_text", currentName));
		view->m_confirmModal.setOkCallback([view]() { view->m_presenter->cancelCurrentObject(); });
		openModal(&view->m_confirmModal);
	}

	void ObjectCancelView::onObjectOverlayPressed(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		auto* overlay = static_cast<OverlayRect*>(lv_event_get_user_data(e));
		if (overlay == nullptr || overlay->owner == nullptr || overlay->owner->m_presenter == nullptr)
		{
			return;
		}
		overlay->owner->m_presenter->onObjectSelected(overlay->index);
	}
} // namespace UI
