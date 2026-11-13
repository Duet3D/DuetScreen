/*
 * LvObj.h
 *
 *  Created on: 2025-06-10
 *      Author: Andy Everitt
 */

#pragma once

#include "Debug.h"
#include "LockWrapper.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/lv_conf_internal.h"
#include <concepts>
#include <functional>
#include <list>
#include <vector>

#define UI_LOCK()                                                                                                      \
	LOG_VERBOSE("UI_LOCK requested by thread {}", Log::GetThreadId());                                                 \
	std::lock_guard<LockableBase(DeadlockDetectingMutex<std::recursive_mutex>)> uiLock(mutexUi);

namespace UI
{
	// Concept to ensure wrappers' Derived expose getRootPtr() returning lv_obj_t*
	template <typename T>
	concept HasGetRootPtr = requires(T& t, const T& ct) {
		{ t.getRootPtr() } -> std::same_as<lv_obj_t*>;
		{ ct.getRootPtr() } -> std::same_as<lv_obj_t*>;
	};

	void lv_timer_delete_safe(lv_timer_t* timer);

	/**
	 * Take `tree` (a widget and everything inside it) out of its input groups, the focused widget last, so focus
	 * moves to a widget outside it. Call before deleting a large tree: deleting the focused widget otherwise moves
	 * focus to the next widget in the group, often the next to be deleted, and each move scrolls that widget into
	 * view, laying out the half-deleted screen again. That is slow, and a layout that never settles hangs.
	 */
	void removeFromInputGroups(lv_obj_t* tree);

	/**
	 * @brief Position and size layout in percentage 0-100
	 */
	struct layout_t
	{
		lv_coord_t x;
		lv_coord_t y;
		lv_coord_t w;
		lv_coord_t h;
	};

	enum class Padding
	{
		ALL,
		HORIZONTAL,
		VERTICAL,
		TOP,
		BOTTOM,
		LEFT,
		RIGHT,
		COLUMN,
		ROW,
	};

	using lv_create_t = std::function<lv_obj_t*(lv_obj_t* parent)>;

	class LvObj
	{
	  public:
		LvObj(lv_create_t initFunc, const std::string& name, LvObj& parent);
		LvObj(lv_create_t initFunc, const std::string& name, LvObj& parent, layout_t layout);
		LvObj(lv_create_t initFunc, const std::string& name)
			: LvObj(initFunc, name, lv_screen_active())
		{
		}

		// Delete copy constructor and assignment operator
		LvObj(const LvObj&) = delete;
		LvObj& operator=(const LvObj&) = delete;

		/* Move constructor and assignment operator */
		LvObj(LvObj&&) noexcept = default;
		LvObj& operator=(LvObj&&) noexcept = default;

		static LvObj* fromPtr(lv_obj_t* obj);

		virtual ~LvObj();

		std::string_view getName() const;

		inline LvObj& getRoot() { return *this; }
		inline const LvObj& getRoot() const { return *this; }

		/**
		 * @return Get the base container for the view
		 */
		inline lv_obj_t* getRootPtr() const { return m_root; }
		explicit operator lv_obj_t*() const { return getRootPtr(); }

		bool operator==(const LvObj& obj) const { return getRootPtr() == obj.getRootPtr(); }

		bool isValid() const { return getRootPtr() != nullptr; }

		/* XML */
		static void registerWidgetXml();

		lv_display_t* getDisplayPtr() const;
		lv_obj_t* getScreenPtr() const;
		LvObj* getParent() const;
		lv_obj_t* getParentPtr() const;
		LvObj* getChild(int32_t id) const;
		lv_obj_t* getChildPtr(int32_t id) const;
		LvObj* getChildByName(std::string_view name) const;
		uint32_t getChildCount() const;
		layout_t getLayout() const;
		layout_t getLayoutPct() const;
		lv_area_t getCoords() const;
		lv_coord_t getX() const;
		lv_coord_t getX2() const;
		lv_coord_t getY() const;
		lv_coord_t getY2() const;
		lv_coord_t getWidth() const;
		lv_coord_t getHeight() const;
		lv_coord_t getContentWidth() const;
		lv_coord_t getContentHeight() const;
		lv_coord_t getSelfWidth() const;
		lv_coord_t getSelfHeight() const;
		lv_style_value_t getStyleProp(lv_style_prop_t prop, lv_part_t part = LV_PART_MAIN) const;

		template <typename F>
			requires(std::is_invocable_v<F, size_t, LvObj&>)
		void iterateChildren(F&& func)
		{
			ZoneScoped;
			UI_LOCK();
			uint32_t count = getChildCount();
			for (uint32_t i = 0; i < count; i++)
			{
				LvObj* child = getChild(i);
				if (child == nullptr)
				{
					continue;
				}
				std::invoke(func, i, *child);
			}
		}

		template <typename F>
			requires(std::is_invocable_v<F, size_t, LvObj&> &&
					 std::is_same_v<std::invoke_result_t<F, size_t, LvObj&>, bool>)
		void iterateChildrenWhile(F&& func)
		{
			ZoneScoped;
			UI_LOCK();
			uint32_t count = getChildCount();
			for (uint32_t i = 0; i < count; i++)
			{
				LvObj* child = getChild(i);
				if (child == nullptr)
				{
					continue;
				}
				if (!std::invoke(func, i, *child))
					break;
			}
		}

		void setUserData(void* user_data);
		void* getUserData() const;

		bool hasFlag(lv_obj_flag_t flag) const;
		bool hasState(lv_state_t state) const;
		bool hasStyle(const lv_style_t* style) const;
		bool hasStyleProp(lv_style_prop_t prop, lv_style_selector_t selector = LV_PART_MAIN) const;

		void setParent(LvObj& parent);
		void setLayoutStyle(lv_layout_t style);
		void setFlexGrow(uint8_t grow);
		void setFlexFlow(lv_flex_flow_t flow);
		void setFlexAlign(lv_flex_align_t main, lv_flex_align_t cross, lv_flex_align_t mid);
		void setGridDsc(const std::span<const int32_t> col_dsc, const std::span<const int32_t> row_dsc);
		void setGridCell(LvObj& obj,
						 lv_grid_align_t x_align,
						 int32_t col_pos,
						 int32_t col_span,
						 lv_grid_align_t y_align,
						 int32_t row_pos,
						 int32_t row_span);
		void setLayout(layout_t layout);
		void setWidth(lv_coord_t width);
		void setHeight(lv_coord_t height);
		void setSize(lv_coord_t width, lv_coord_t height);
		void setMinWidth(lv_coord_t width, lv_style_selector_t selector = LV_PART_MAIN);
		void setMinHeight(lv_coord_t height, lv_style_selector_t selector = LV_PART_MAIN);
		void setMaxWidth(lv_coord_t width, lv_style_selector_t selector = LV_PART_MAIN);
		void setMaxHeight(lv_coord_t height, lv_style_selector_t selector = LV_PART_MAIN);
		void setX(lv_coord_t x);
		void setY(lv_coord_t y);
		void setPos(lv_coord_t x, lv_coord_t y);
		void setFlag(lv_obj_flag_t flag, bool enable, bool recursive = false);
		void setState(lv_state_t state, bool enable, bool recursive = false);
		void setAlign(lv_align_t align);
		void setAlign(lv_align_t align, lv_coord_t x, lv_coord_t y);
		void setAlignTo(LvObj& target, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
		void setLocalStyleProp(lv_style_prop_t prop,
							   lv_style_value_t value,
							   lv_style_selector_t selector = LV_PART_MAIN);
		bool removeLocalStyleProp(lv_style_prop_t prop, lv_style_selector_t selector = LV_PART_MAIN);

		void updateLayout();
		bool refreshSelfSize();
		void invalidate();
		void markLayoutDirty();

		void setExtDrawSize(int32_t size);
		void setExtClickArea(int32_t size);
		void getClickArea(lv_area_t* area) const;

		void setScrollbarMode(lv_scrollbar_mode_t mode);
		lv_scrollbar_mode_t getScrollbarMode() const;
		void scrollBy(int32_t dx, int32_t dy, lv_anim_enable_t anim = LV_ANIM_OFF);
		void scrollByBounded(int32_t dx, int32_t dy, lv_anim_enable_t anim = LV_ANIM_OFF);
		void scrollToX(lv_coord_t x, lv_anim_enable_t anim = LV_ANIM_OFF);
		void scrollToY(lv_coord_t y, lv_anim_enable_t anim = LV_ANIM_OFF);
		void setScrollDir(lv_dir_t dir);
		lv_coord_t getScrollLeft() const;
		lv_coord_t getScrollRight() const;
		lv_coord_t getScrollTop() const;
		lv_coord_t getScrollBottom() const;

		/* Styling */

		void addStyle(const lv_style_t* style,
					  const lv_style_selector_t selector = LV_PART_MAIN,
					  bool recursive = false);
		void removeStyle(const lv_style_t* style,
						 const lv_style_selector_t selector = LV_PART_MAIN,
						 bool recursive = false);
		void setStylePad(lv_coord_t pad, lv_style_selector_t selector = LV_PART_MAIN, Padding type = Padding::ALL);
		void setStyleBgColor(lv_color_t color, lv_style_selector_t selector = LV_PART_MAIN);
		void setStyleBgOpa(lv_opa_t opa, lv_style_selector_t selector = LV_PART_MAIN);
		void setStyleRecolor(lv_color_t color, lv_style_selector_t selector = LV_PART_MAIN);
		void setStyleRecolorOpa(lv_opa_t opa, lv_style_selector_t selector = LV_PART_MAIN);
		void setStyleTextAlign(lv_text_align_t align, lv_style_selector_t selector = LV_PART_MAIN);
		void setStyleFont(const lv_font_t* font, lv_style_selector_t selector = LV_PART_MAIN);

		/* Events */

		lv_event_dsc_t* addEventCallback(lv_event_cb_t cb, lv_event_code_t code, void* userData);
		void addEventCallback(const std::function<void(lv_event_t*)>& cb, lv_event_code_t code);
		bool removeEvent(uint32_t index);
		uint32_t removeEventCallback(lv_event_cb_t cb);
		uint32_t removeEventCallbackWithUserData(lv_event_cb_t cb, void* userData);
		uint32_t getEventCount();
		lv_result_t sendEvent(lv_event_code_t code, void* param = nullptr);

		void moveToFront();
		void moveToBack();
		void moveToIndex(int32_t index);
		void clearChildren();

		void setVisible(bool display, bool move_to_front = false);
		void show(bool move_to_front = false);
		void hide(bool move_to_back = false);
		bool isVisible();

		/** Whether neither this nor anything it is inside is hidden (it may still be scrolled out of sight) */
		bool isShownOnScreen() const;

		virtual bool back();

	  protected:
		LvObj(lv_create_t initFunc, const std::string& name, lv_obj_t* parent);

		/** Refer to `existing` without owning it (see LvObjRef) */
		struct BorrowTag
		{
		};
		LvObj(BorrowTag, lv_obj_t* existing);

		virtual void onInit() {}
		virtual void onShow() {}
		virtual void onHide() {}

		virtual void showInner();
		virtual void hideInner();

		/**
		 * Call `func` with each LvObj nested in `obj`: its children that are LvObjs, and those inside its plain widgets
		 * (an XML <lv_obj>, say), which have no LvObj to pass a show or hide on. Unless `throughHidden`, hidden plain
		 * widgets are skipped, as nothing in them is shown.
		 */
		static void forEachNested(lv_obj_t* obj, bool throughHidden, const std::function<void(LvObj&)>& func);

		lv_obj_t* m_root;
		std::string m_name;

		uint8_t m_initialized : 1 = 0; // 1 = initialized, 0 = not initialized
		uint8_t m_showing : 1 = 0;	   // 1 = showing, 0 = hidden
		uint8_t m_hidding : 1 = 0;	   // 1 = hiding, 0 = not hiding
		void* m_userData = nullptr;

		struct EventCallbackInfo
		{
			std::function<void(lv_event_t*)> cb;
			lv_event_code_t code;
		};
		std::vector<EventCallbackInfo> m_eventCallbacks;
		static void genericEventCallback(lv_event_t* e);
	};
	/**
	 * A handle to a widget something else created and owns, such as the XML engine, so C++ views can be built inside
	 * it:
	 *
	 *     LvObjRef parent(xmlCreatedObj);
	 *     auto* view = new TemperatureView("temperature", parent);
	 *
	 * It never deletes the widget and leaves the widget's user data alone. Use it only while constructing: views must
	 * not keep a reference to their parent.
	 */
	class LvObjRef : public LvObj
	{
	  public:
		/** Refer to `obj` */
		explicit LvObjRef(lv_obj_t* obj);

		/** Let go of the widget without deleting it */
		~LvObjRef() override;
	};
} // namespace UI
