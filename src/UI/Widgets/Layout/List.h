/*
 * List.h
 *
 *  Created on: 2025-05-02
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/LvContainer.h"
#include "UI/Lvgl/LvLabel.h"
#include "UI/Styles/Styles.h"
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace UI
{
	// Concept to validate a list item constructor for T
	// Parameter order: F (callable), then T (constructed type)
	template <typename F, typename T>
	concept ListItemConstructor = requires(F&& f, size_t index, LvObj& parent) {
		{ std::invoke(std::forward<F>(f), index, parent) } -> std::same_as<std::unique_ptr<T>>;
	};

	// Helper: true when Args... is exactly one type that itself is a valid
	// constructor functor for T (invocable with (size_t, LvObj&) returning unique_ptr<T>).
	template <typename T, typename U>
	concept SingleCtorArgFor = ListItemConstructor<std::decay_t<U>, T>;

	template <typename T, typename... U>
	concept NotSingleCtorArgFor = !(sizeof...(U) == 1 && (SingleCtorArgFor<T, U> && ...));

	class ListItem : public LvContainer
	{
	  public:
		ListItem(size_t index, LvObj& parent)
			: LvContainer(fmt::format("{}", index), parent)
			, m_index(index)
		{
			ZoneScoped;
			// addStyle(Themes::getLvglStyles().bg_color_list_item, 0);
			// addStyle(Themes::getLvglStyles().bg_color_secondary, LV_STATE_CHECKED);
		}

		size_t getIndex() const { return m_index; }

	  private:
		const size_t m_index;
	};

	template <typename T>
	// requires(std::is_base_of_v<LvObj, T>) // Disabled because it prevents forward declaration use
	class List : public LvObj
	{
		using TPtr = std::unique_ptr<T>;
		using TRef = T&;

	  public:
		List(const std::string& name, LvObj& parent)
			: LvObj(lv_obj_create, name, parent)
			, m_header("header", getRoot())
			, m_title("title", m_header)
			, m_listCont("list", getRoot())
		{
			ZoneScoped;
			setFlexFlow(LV_FLEX_FLOW_COLUMN);

			m_header.setMinWidth(LV_SIZE_CONTENT);
			m_header.setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_header.setFlexFlow(LV_FLEX_FLOW_ROW);
			m_header.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			m_header.addStyle(Themes::getLvglStyles().bg_color_header);
			m_header.addStyle(Themes::getComponentStyles().list_header);

			m_listCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
			m_listCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
			m_listCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);

			m_title.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			m_title.addStyle(Themes::getComponentStyles().list_title);

			showTitle(false);
		}

		LvContainer& getHeader() { return m_header; }
		LvLabel& getTitle() { return m_title; }
		LvContainer& getListContainer() { return m_listCont; }

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
			else if (m_header.getChildCount() <= 1) // Only the title is present
			{
				showHeader(false);
			}
		}

		void setListPad(lv_coord_t pad, lv_style_selector_t selector = LV_PART_MAIN, Padding type = Padding::ALL)
		{
			m_listCont.setStylePad(pad, selector, type);
		}

		void setListFlow(lv_flex_flow_t flow) { m_listCont.setFlexFlow(flow); }

		void setListGrow(const uint8_t grow) { m_listCont.setFlexGrow(grow); }

		void addListStyle(const lv_style_t* style, lv_style_selector_t selector = LV_PART_MAIN)
		{
			m_listCont.addStyle(style, selector);
		}

		void setListSize(const lv_coord_t w, const lv_coord_t h) { m_listCont.setSize(w, h); }

		void clear()
		{
			ZoneScoped;
			m_list.clear();
		}

		TRef addItem()
		{
			ZoneScoped;
			UI_LOCK();
			LOG_DBG("Adding item to list \"{:s}\"", getName());
			auto item = std::make_unique<T>(getItemCount(), m_listCont);
			m_list.push_back(std::move(item));
			return *m_list.back();
		}

		template <typename F>
			requires ListItemConstructor<F, T>
		TRef addItem(F&& constructor)
		{
			ZoneScoped;
			UI_LOCK();
			LOG_DBG("Adding item to list \"{:s}\"", getName());
			auto item = std::invoke(constructor, getItemCount(), m_listCont);
			m_list.push_back(std::move(item));
			return *m_list.back();
		}

		template <typename F>
			requires ListItemConstructor<F, T>
		size_t setItemCount(const size_t count, F&& constructor)
		{
			ZoneScoped;
			UI_LOCK();
			const size_t currentCount = getItemCount();
			if (count == currentCount)
			{
				return 0;
			}

			if (count < currentCount)
			{
				LOG_DBG("Resizing list \"{:s}\" down from {:d} to {:d}", getName(), currentCount, count);
				m_list.resize(count);
			}
			else
			{
				LOG_DBG("Resizing list \"{:s}\" up from {:d} to {:d}", getName(), currentCount, count);
				m_list.reserve(count);
				const bool isVisible = m_listCont.isVisible();
				m_listCont.hide(); // this is to prevent layout calculations for every item added
				for (size_t i = currentCount; i < count; i++)
				{
					m_list.emplace_back(std::invoke(constructor, i, m_listCont));
				}
				m_listCont.setVisible(isVisible);
			}

			return count > currentCount ? count - currentCount : 0;
		}

		template <typename Class, typename... Args>
		size_t setItemCount(size_t count,
							Class* instance,
							TPtr (Class::*constructor)(const size_t index, LvObj& parent, Args...),
							Args&&... args)
		{
			ZoneScoped;
			UI_LOCK();
			const size_t currentCount = getItemCount();
			if (count == currentCount)
			{
				return 0;
			}

			if (count < currentCount)
			{
				LOG_DBG("Resizing list \"{:s}\" down from {:d} to {:d}", getName(), currentCount, count);
				m_list.resize(count);
			}
			else
			{
				LOG_DBG("Resizing list \"{:s}\" up from {:d} to {:d}", getName(), currentCount, count);
				m_list.reserve(count);
				const bool isVisible = m_listCont.isVisible();
				m_listCont.hide(); // this is to prevent layout calculations for every item added
				for (size_t i = currentCount; i < count; i++)
				{
					m_list.emplace_back((instance->*constructor)(i, m_listCont, std::forward<Args>(args)...));
				}
				m_listCont.setVisible(isVisible);
			}

			return count > currentCount ? count - currentCount : 0;
		}

		template <typename... Args>
			requires std::constructible_from<T, size_t, LvObj&, Args...> && NotSingleCtorArgFor<T, Args...>
		size_t setItemCount(const size_t count, Args&&... args)
		{
			ZoneScoped;
			UI_LOCK();
			const size_t currentCount = getItemCount();
			if (count == currentCount)
			{
				return 0;
			}

			if (count < currentCount)
			{
				LOG_DBG("Resizing list \"{:s}\" down from {:d} to {:d}", getName(), currentCount, count);
				m_list.resize(count);
			}
			else
			{
				LOG_DBG("Resizing list \"{:s}\" up from {:d} to {:d}", getName(), currentCount, count);
				m_list.reserve(count);
				const bool isVisible = m_listCont.isVisible();
				m_listCont.hide(); // this is to prevent layout calculations for every item added
				for (size_t i = currentCount; i < count; i++)
				{
					m_list.emplace_back(std::make_unique<T>(i, m_listCont, std::forward<Args>(args)...));
				}
				m_listCont.setVisible(isVisible);
			}

			return count > currentCount ? count - currentCount : 0;
		}

		size_t getItemCount() const
		{
			ZoneScoped;
			return m_list.size();
		}

		T* getItem(const size_t index) const
		{
			ZoneScoped;
			UI_LOCK();
			if (index >= m_list.size())
			{
				return nullptr;
			}
			return m_list.at(index).get();
		}

		const std::vector<TPtr>& getItems() const
		{
			ZoneScoped;
			return m_list;
		}

		void iterateListItems(const std::function<void(size_t index, TRef item)>& func)
		{
			ZoneScoped;
			UI_LOCK();
			for (size_t i = 0; i < m_list.size(); i++)
			{
				auto& item = m_list.at(i);
				if (!item)
				{
					continue;
				}
				func(i, *item);
			}
		}

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
