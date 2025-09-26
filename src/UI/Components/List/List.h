/*
 * List.h
 *
 *  Created on: 2025-05-02
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/LVGL/LvObj.h"
#include "UI/Styles/Styles.h"
#include <memory>
#include <vector>

namespace UI
{
	class ListItem : public LvObj
	{
	  public:
		ListItem(size_t index, LvObj& parent)
			: LvObj(lv_obj_create, fmt::format("{}", index), parent)
			, m_index(index)
		{
			// addStyle(Themes::getLvglStyles().bg_color_list_item, 0);
			// addStyle(Themes::getLvglStyles().bg_color_secondary, LV_STATE_CHECKED);
		}

		const size_t getIndex() const { return m_index; }

	  private:
		const size_t m_index;
	};

	template <typename T>
	// requires(std::is_base_of_v<LvObj, T>)
	class List : public LvObj
	{
		using TPtr = std::shared_ptr<T>;

	  public:
		List(const std::string& name, LvObj& parent)
			: LvObj(lv_obj_create, name, parent)
			, m_header("header", getRoot())
			, m_title("title", m_header)
			, m_listCont("list", getRoot())
		{
			setFlexFlow(LV_FLEX_FLOW_COLUMN);

			m_header.setMinWidth(LV_SIZE_CONTENT);
			m_header.setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_header.setFlexFlow(LV_FLEX_FLOW_ROW);
			m_header.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			m_header.addStyle(Themes::getLvglStyles().bg_color_header, LV_PART_MAIN);

			m_listCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
			m_listCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
			m_listCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);

			m_title.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

			showTitle(false);
		}

		LvContainer& getHeader() { return m_header; }
		LvLabel& getTitle() { return m_title; }
		LvContainer& getListContainer() { return m_listCont; }

		void setTitle(const std::string& title)
		{
			UI_LOCK();
			m_title.setText(title);
			showTitle(!title.empty());
		}

		void showHeader(bool show)
		{
			UI_LOCK();
			m_header.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
		}

		void showTitle(bool show)
		{
			UI_LOCK();
			m_title.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
			if (show)
			{
				showHeader(true);
			}
			else if (m_header.getChildCnt() <= 1) // Only the title is present
			{
				showHeader(false);
			}
		}

		void setListPad(lv_coord_t pad, lv_style_selector_t selector = LV_PART_MAIN, Padding type = Padding::ALL)
		{
			UI_LOCK();
			switch (type)
			{
			case Padding::ALL:
				lv_obj_set_style_pad_all(m_listCont, pad, selector);
				break;
			case Padding::LEFT:
				lv_obj_set_style_pad_left(m_listCont, pad, selector);
				break;
			case Padding::RIGHT:
				lv_obj_set_style_pad_right(m_listCont, pad, selector);
				break;
			case Padding::TOP:
				lv_obj_set_style_pad_top(m_listCont, pad, selector);
				break;
			case Padding::BOTTOM:
				lv_obj_set_style_pad_bottom(m_listCont, pad, selector);
				break;
			case Padding::COLUMN:
				lv_obj_set_style_pad_column(m_listCont, pad, selector);
				break;
			case Padding::ROW:
				lv_obj_set_style_pad_row(m_listCont, pad, selector);
				break;
			case Padding::HORIZONTAL:
				lv_obj_set_style_pad_hor(m_listCont, pad, selector);
				break;
			case Padding::VERTICAL:
				lv_obj_set_style_pad_ver(m_listCont, pad, selector);
				break;
			default:
				LOG_WARN("Unknown padding type");
				break;
			}
		}

		void setListFlow(lv_flex_flow_t flow)
		{
			UI_LOCK();
			m_listCont.setFlexFlow(flow);
		}

		void setListGrow(const uint8_t grow)
		{
			UI_LOCK();
			m_listCont.setFlexGrow(grow);
		}

		void addListStyle(const lv_style_t* style, lv_style_selector_t selector = LV_PART_MAIN)
		{
			UI_LOCK();
			m_listCont.addStyle(style, selector);
		}

		void setListSize(const lv_coord_t w, const lv_coord_t h)
		{
			UI_LOCK();
			m_listCont.setSize(w, h);
		}

		void clear()
		{
			UI_LOCK();
			m_list.clear();
		}

		TPtr addItem()
		{
			UI_LOCK();
			auto item = std::make_shared<T>(getItemCount(), m_listCont);
			m_list.push_back(item);
			return item;
		}

		TPtr addItem(std::function<TPtr(size_t, LvObj&)> constructor)
		{
			UI_LOCK();
			auto item = constructor(getItemCount(), m_listCont);
			m_list.push_back(item);
			return item;
		}

		void setItemCount(const size_t count, std::function<TPtr(size_t, LvObj&)> constructor)
		{
			UI_LOCK();
			const size_t currentCount = getItemCount();
			if (count == currentCount)
			{
				return;
			}

			if (count < currentCount)
			{
				m_list.resize(count);
			}

			m_list.reserve(count);
			for (size_t i = currentCount; i < count; i++)
			{
				m_list.emplace_back(constructor(i, m_listCont));
			}
		}

		template <typename F, typename = std::enable_if_t<std::is_invocable_r_v<TPtr, F, size_t, LvObj&>>>
		void setItemCount(const size_t count, F&& constructor)
		{
			setItemCount(count, std::function<TPtr(size_t, LvObj&)>(std::forward<F>(constructor)));
		}

		template <typename Class, typename... Args>
		void setItemCount(size_t count,
						  Class* instance,
						  TPtr (Class::*constructor)(const size_t index, LvObj& parent, Args...),
						  Args&&... args)
		{
			UI_LOCK();
			const size_t currentCount = getItemCount();
			if (count == currentCount)
			{
				return;
			}

			if (count < currentCount)
			{
				m_list.resize(count);
			}

			m_list.reserve(count);
			for (size_t i = currentCount; i < count; i++)
			{
				m_list.emplace_back((instance->*constructor)(i, m_listCont, std::forward<Args>(args)...));
			}
		}

		template <typename... Args,
				  typename = std::enable_if_t<sizeof...(Args) != 1 ||
											  !std::is_invocable_r_v<TPtr, std::decay_t<Args>..., size_t, LvObj&>>>
		void setItemCount(const size_t count, Args&&... args)
		{
			UI_LOCK();
			const size_t currentCount = getItemCount();
			if (count == currentCount)
			{
				return;
			}

			if (count < currentCount)
			{
				m_list.resize(count);
			}

			m_list.reserve(count);
			for (size_t i = currentCount; i < count; i++)
			{
				m_list.emplace_back(std::make_shared<T>(i, m_listCont, std::forward<Args>(args)...));
			}
		}

		const size_t getItemCount() const { return m_list.size(); }

		TPtr getItem(const size_t index) const
		{
			UI_LOCK();
			if (index >= m_list.size())
			{
				return nullptr;
			}
			return m_list.at(index);
		}

		const std::vector<TPtr>& getItems() const { return m_list; }

		auto begin() { return m_list.begin(); }
		auto end() { return m_list.end(); }
		auto begin() const { return m_list.begin(); }
		auto end() const { return m_list.end(); }

	  private:
		LvContainer m_header;
		LvLabel m_title;
		LvContainer m_listCont;

		std::vector<TPtr> m_list;
	};
} // namespace UI
