/*
 * LazyList.h
 *
 *  Created on: 2025-12-01
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/LvContainer.h"
#include "UI/Lvgl/LvLabel.h"
#include "UI/Styles/Styles.h"
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace UI
{
	template <typename LazyT>
	class LazyList;

	// Concept to validate a list item constructor for T
	// Parameter order: F (callable), then T (constructed type)
	template <typename F, typename T>
	concept LazyListItemConstructor = requires(F&& f, size_t index, LvObj& parent) {
		{ std::invoke(std::forward<F>(f), index) } -> std::same_as<T>;
	};

	template <typename T>
		requires(std::is_base_of_v<LvObj, T>)
	class LazyObj
	{
	  public:
		using ObjT = T;

		LvObj& getLazyList() const
		{
			assert(m_lazyList != nullptr);
			return *m_lazyList;
		}

		/**
		 * @brief Calculate the width of this item.
		 * @return Width in pixels.
		 */
		virtual int32_t getWidth() const = 0;

		/**
		 * @brief Calculate the height of this item.
		 * @return Height in pixels.
		 */
		virtual int32_t getHeight() const = 0;

		/**
		 * @brief Create the LVGL object for this item.
		 * @param index The index of the item.
		 * @param parent The parent object (the list).
		 * @return Pointer to the created object.
		 */
		std::unique_ptr<ObjT> create(size_t index, LvObj& parent)
		{
			ZoneScoped;
			auto obj = std::make_unique<ObjT>(fmt::format("lazy_item_{:d}", index), parent);
			obj->setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
			return obj;
		}

		/**
		 * @brief Update the LVGL object with data from this LazyObj.
		 * @param index The index of the LazyObj.
		 * @param obj The LVGL object to update (created by create()).
		 */
		virtual void update(size_t index, ObjT& obj) = 0;

		/**
		 * @brief Called to determine if the item is visible and should be considered when rendering the list
		 * @return `true` if the item is visible
		 */
		virtual bool isVisible() const { return true; }

	  private:
		template <typename LazyT>
		friend class LazyList;

		void setLazyList(LvObj& list) { m_lazyList = &list; }

		LvObj* m_lazyList = nullptr;
	};

	template <typename LazyT>
	class LazyList : public LvContainer
	{
		using LazyTPtr = std::unique_ptr<LazyT>;
		using LazyTRef = LazyT&;
		using T = LazyT::ObjT;
		using TPtr = std::unique_ptr<T>;
		using TRef = T&;

	  public:
		LazyList(const std::string& name, LvObj& parent)
			: LvContainer(name, parent)
			, m_header("header", getRoot())
			, m_title("title", m_header)
			, m_listCont("list", getRoot())
			, m_sizer(lv_obj_create, "sizer", m_listCont)
		{
			ZoneScoped;
			// Setup layout similar to List
			setFlexFlow(LV_FLEX_FLOW_COLUMN);

			m_header.setMinWidth(LV_SIZE_CONTENT);
			m_header.setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_header.setFlexFlow(LV_FLEX_FLOW_ROW);
			m_header.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			m_header.addStyle(Themes::getLvglStyles().bg_color_header, LV_PART_MAIN);

			// List container (scrollable)
			setListFlow(LV_FLEX_FLOW_COLUMN);
			m_listCont.setSize(LV_PCT(100), LV_PCT(100));
			m_listCont.setFlexGrow(1);
			m_listCont.setLayoutStyle(LV_LAYOUT_NONE);

			m_title.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			showTitle(false);

			// Sizer for scrolling
			m_sizer.setSize(1, 1);
			m_sizer.setFlag(LV_OBJ_FLAG_CLICKABLE, false);
			m_sizer.setFlag(LV_OBJ_FLAG_CLICK_FOCUSABLE, false);

			// Events
			addEventCallback(
				[this](lv_event_t*)
				{
					auto width = lv_obj_get_style_clamped_width(getRootPtr());
					auto height = lv_obj_get_style_clamped_height(getRootPtr());

					m_listCont.setWidth(width == LV_SIZE_CONTENT ? LV_SIZE_CONTENT : LV_PCT(100));
					m_listCont.setHeight(height == LV_SIZE_CONTENT ? LV_SIZE_CONTENT : LV_PCT(100));
					m_listCont.setFlexGrow(height == LV_SIZE_CONTENT ? 0 : 1);
				},
				LV_EVENT_SIZE_CHANGED);
			m_listCont.addEventCallback([this](lv_event_t*) { updateVisibleItems(); }, LV_EVENT_SCROLL);
			m_listCont.addEventCallback([this](lv_event_t*) { updateVisibleItems(); }, LV_EVENT_SIZE_CHANGED);
		}

		// List Interface methods
		LvContainer& getHeader() { return m_header; }
		LvLabel& getTitle() { return m_title; }
		LvContainer& getListContainer() { return m_listCont; }

		auto& getVisibleItems() { return m_items; }
		auto& getLazyItems() { return m_pool; }

		size_t getVisibleItemCount() const { return m_items.size(); }
		size_t getLazyItemCount() const { return m_pool.size(); }

		auto getVisibleItem(size_t index) const { return m_items.at(index).get(); }
		auto getLazyItem(size_t index) const { return m_pool.at(index).get(); }

		void setTitle(std::string_view title)
		{
			ZoneScoped;
			UI_LOCK();
			m_title.setText(title);
			showTitle(!title.empty());
		}

		void showHeader(bool show)
		{
			ZoneScoped;
			UI_LOCK();
			m_header.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
		}

		void showTitle(bool show)
		{
			ZoneScoped;
			UI_LOCK();
			m_title.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
			if (show)
			{
				showHeader(true);
			}
			else if (m_header.getChildCount() <= 1)
			{
				showHeader(false);
			}
		}

		void setListPad(lv_coord_t pad, lv_style_selector_t selector = LV_PART_MAIN, Padding type = Padding::ALL)
		{
			m_listCont.setStylePad(pad, selector, type);
			refresh();
		}

		void setListFlow(lv_flex_flow_t flow)
		{
			m_listFlow = flow;
			switch (flow)
			{
			case LV_FLEX_FLOW_ROW:
			case LV_FLEX_FLOW_ROW_REVERSE:
			case LV_FLEX_FLOW_COLUMN_WRAP:
			case LV_FLEX_FLOW_COLUMN_WRAP_REVERSE:
				m_listCont.setScrollDir(LV_DIR_HOR);
				break;
			case LV_FLEX_FLOW_ROW_WRAP:
			case LV_FLEX_FLOW_ROW_WRAP_REVERSE:
			case LV_FLEX_FLOW_COLUMN:
			case LV_FLEX_FLOW_COLUMN_REVERSE:
				m_listCont.setScrollDir(LV_DIR_VER);
			}
			refresh();
		}

		void setListGrow(const uint8_t grow) { m_listCont.setFlexGrow(grow); }

		void addListStyle(const lv_style_t* style, lv_style_selector_t selector = LV_PART_MAIN)
		{
			m_listCont.addStyle(style, selector);
			refresh();
		}

		void setListSize(const lv_coord_t w, const lv_coord_t h)
		{
			m_listCont.setSize(w, h);
			refresh();
		}

		void clear()
		{
			ZoneScoped;
			m_items.clear();
			m_pool.clear();
			m_sizer.setSize(0, 0);
		}

		template <typename F>
			requires LazyListItemConstructor<F, LazyTPtr>
		size_t setItemCount(const size_t count, F&& constructor)
		{
			ZoneScoped;
			UI_LOCK();
			const size_t currentCount = m_pool.size();
			if (count == currentCount)
			{
				return 0;
			}

			if (count < currentCount)
			{
				m_pool.resize(count);
			}
			else
			{
				m_pool.reserve(count);
				for (size_t i = currentCount; i < count; i++)
				{
					auto lazyObj = std::invoke(constructor, i);
					lazyObj->setLazyList(*this);
					m_pool.emplace_back(std::move(lazyObj));
				}
			}

			for (auto& item : m_pool)
			{
				if (item)
				{
					item->setLazyList(*this);
				}
			}
			refresh();
			return count > currentCount ? count - currentCount : 0;
		}

		void refresh()
		{
			ZoneScoped;
			updateLayout();
			const bool column = isColumnFlow(m_listFlow);
			const bool wrap = isWrapFlow(m_listFlow);

			const int32_t mainPad = column ? m_listCont.getStyleProp(LV_STYLE_PAD_ROW).num
										   : m_listCont.getStyleProp(LV_STYLE_PAD_COLUMN).num;
			const int32_t crossPad = column ? m_listCont.getStyleProp(LV_STYLE_PAD_COLUMN).num
											: m_listCont.getStyleProp(LV_STYLE_PAD_ROW).num;
			const int32_t mainLimit =
				std::max(0, column ? m_listCont.getContentHeight() : m_listCont.getContentWidth());

			if (wrap && mainLimit > 0)
			{
				int32_t lineMainUsed = 0;
				int32_t lineCrossMax = 0;
				int32_t crossStart = 0;

				for (const auto& item : m_pool)
				{
					if (!item->isVisible())
					{
						continue;
					}

					const int32_t mainSize = column ? item->getHeight() : item->getWidth();
					const int32_t crossSize = column ? item->getWidth() : item->getHeight();

					if (lineMainUsed > 0 && lineMainUsed + mainPad + mainSize > mainLimit)
					{
						crossStart += lineCrossMax + crossPad;
						lineMainUsed = 0;
						lineCrossMax = 0;
					}

					lineMainUsed += (lineMainUsed > 0 ? mainPad : 0) + mainSize;
					lineCrossMax = std::max(lineCrossMax, crossSize);
				}

				const int32_t totalCross = (lineCrossMax > 0) ? (crossStart + lineCrossMax) : 0;
				if (column)
				{
					m_sizer.setSize(totalCross, 1);
				}
				else
				{
					m_sizer.setSize(1, totalCross);
				}
			}
			else
			{
				int32_t totalMain = 0;
				bool first = true;
				for (const auto& item : m_pool)
				{
					if (!item->isVisible())
					{
						continue;
					}
					totalMain += column ? item->getHeight() : item->getWidth();
					if (!first)
					{
						totalMain += mainPad;
					}
					first = false;
				}

				if (column)
				{
					m_sizer.setSize(1, totalMain);
				}
				else
				{
					m_sizer.setSize(totalMain, 1);
				}
			}

			updateVisibleItems();
		}

	  protected:
		void updateVisibleItems()
		{
			ZoneScoped;
			const bool column = isColumnFlow(m_listFlow);
			const bool wrap = isWrapFlow(m_listFlow);

			const int32_t mainPad = column ? m_listCont.getStyleProp(LV_STYLE_PAD_ROW).num
										   : m_listCont.getStyleProp(LV_STYLE_PAD_COLUMN).num;
			const int32_t crossPad = column ? m_listCont.getStyleProp(LV_STYLE_PAD_COLUMN).num
											: m_listCont.getStyleProp(LV_STYLE_PAD_ROW).num;

			const lv_coord_t viewportMainStart = column ? m_listCont.getScrollTop() : m_listCont.getScrollLeft();
			const lv_coord_t viewportMainSize = column ? m_listCont.getContentHeight() : m_listCont.getContentWidth();
			const lv_coord_t viewportMainEnd = viewportMainStart + viewportMainSize;

			const lv_coord_t viewportCrossStart = column ? m_listCont.getScrollLeft() : m_listCont.getScrollTop();
			const lv_coord_t viewportCrossSize = column ? m_listCont.getContentWidth() : m_listCont.getContentHeight();
			const lv_coord_t viewportCrossEnd = viewportCrossStart + viewportCrossSize;

			const int32_t mainLimit = std::max(0, viewportMainSize);

			struct item_info_t
			{
				size_t index;
				lv_coord_t mainPos;
				lv_coord_t crossPos;
				lv_coord_t mainSize;
				lv_coord_t crossSize;
			};

			std::vector<item_info_t> visibleItems;
			lv_coord_t itemMainStart = 0;
			lv_coord_t lineCrossStart = 0;
			lv_coord_t lineMainUsed = 0;
			lv_coord_t lineCrossMax = 0;

			for (size_t i = 0; i < m_pool.size(); ++i)
			{
				const LazyTPtr& item = m_pool.at(i);
				if (!item->isVisible())
				{
					continue;
				}

				const lv_coord_t mainSize = column ? item->getHeight() : item->getWidth();
				const lv_coord_t crossSize =
					(wrap && mainLimit > 0) ? (column ? item->getWidth() : item->getHeight()) : 0;

				lv_coord_t itemMainPos = itemMainStart;
				lv_coord_t itemCrossPos = lineCrossStart;

				if (wrap && mainLimit > 0)
				{
					if (lineMainUsed > 0 && lineMainUsed + mainPad + mainSize > mainLimit)
					{
						lineCrossStart += lineCrossMax + crossPad;
						lineMainUsed = 0;
						lineCrossMax = 0;
					}

					itemMainPos = lineMainUsed + (lineMainUsed > 0 ? mainPad : 0);
					itemCrossPos = lineCrossStart;
					lineMainUsed = itemMainPos + mainSize;
					lineCrossMax = std::max(lineCrossMax, crossSize);
				}
				else
				{
					itemMainPos = itemMainStart;
					itemMainStart += mainSize + mainPad;
				}

				const lv_coord_t visibleStart = wrap ? itemCrossPos : itemMainPos;
				const lv_coord_t visibleEnd = visibleStart + (wrap ? crossSize : mainSize);
				const lv_coord_t viewportStart = wrap ? viewportCrossStart : viewportMainStart;
				const lv_coord_t viewportEnd = wrap ? viewportCrossEnd : viewportMainEnd;

				if (visibleEnd >= viewportStart && visibleStart <= viewportEnd)
				{
					visibleItems.push_back({i, itemMainPos, itemCrossPos, mainSize, crossSize});
				}
			}

			// Iterate visible indices
			if (visibleItems.size() > m_items.size())
			{
				LOG_DBG(
					"Increasing '{:s}' visible items: {:d} -> {:d}", getName(), m_items.size(), visibleItems.size());
				m_items.resize(visibleItems.size());
			}
			for (size_t i = 0; i < visibleItems.size(); ++i)
			{
				item_info_t info = visibleItems[i]; // index in m_pool

				LazyTRef lazyObj = *m_pool.at(info.index);
				TPtr& objPtr = m_items.at(i);

				if (!objPtr)
				{
					objPtr = std::move(lazyObj.create(info.index, m_listCont));
					objPtr->setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);
				}
				if (column)
				{
					objPtr->setX(wrap ? info.crossPos : 0);
					objPtr->setY(info.mainPos);
					objPtr->setHeight(info.mainSize);
					if (wrap)
					{
						objPtr->setWidth(info.crossSize);
					}
				}
				else
				{
					objPtr->setX(info.mainPos);
					objPtr->setY(wrap ? info.crossPos : 0);
					objPtr->setWidth(info.mainSize);
					if (wrap)
					{
						objPtr->setHeight(info.crossSize);
					}
				}
				objPtr->setFlag(LV_OBJ_FLAG_HIDDEN, false);
				lazyObj.update(info.index, *objPtr);
			}
			for (size_t i = visibleItems.size(); i < m_items.size(); ++i)
			{
				TPtr& objPtr = m_items.at(i);

				if (!objPtr)
				{
					continue;
				}
				LOG_DBG("Hiding item {:d}", i);
				objPtr->setFlag(LV_OBJ_FLAG_HIDDEN, true);
			}
		}

	  private:
		static bool isColumnFlow(lv_flex_flow_t flow)
		{
			return flow == LV_FLEX_FLOW_COLUMN || flow == LV_FLEX_FLOW_COLUMN_REVERSE ||
				   flow == LV_FLEX_FLOW_COLUMN_WRAP || flow == LV_FLEX_FLOW_COLUMN_WRAP_REVERSE;
		}

		static bool isWrapFlow(lv_flex_flow_t flow)
		{
			return flow == LV_FLEX_FLOW_ROW_WRAP || flow == LV_FLEX_FLOW_ROW_WRAP_REVERSE ||
				   flow == LV_FLEX_FLOW_COLUMN_WRAP || flow == LV_FLEX_FLOW_COLUMN_WRAP_REVERSE;
		}

		LvContainer m_header;
		LvLabel m_title;
		LvContainer m_listCont;
		LvObj m_sizer;
		lv_flex_flow_t m_listFlow = LV_FLEX_FLOW_COLUMN;

		std::vector<TPtr> m_items;	  // the actual lvgl objects for visible items
		std::vector<LazyTPtr> m_pool; // pool of all lazy objects
	};

} // namespace UI
