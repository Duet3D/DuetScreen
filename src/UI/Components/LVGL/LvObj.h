/*
 * LvObj.h
 *
 *  Created on: 2025-06-10
 *      Author: Andy Everitt
 */

#pragma once

#include "LockWrapper.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/lv_conf_internal.h"
#include <functional>

namespace UI
{
	void lv_timer_delete_safe(lv_timer_t* timer);

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
		template <typename F>
		struct callback_t;

		template <typename Ret, typename... Args>
		struct callback_t<Ret(Args...)>
		{
			std::function<Ret(Args..., void* user_data)> func;
			void* user_data = nullptr;

			callback_t() = default;

			/**
			 * @brief Construct a callback with a function and optional user data
			 * @param f Function to call when the callback is executed
			 * @param data Optional user data to pass to the function
			 */
			callback_t(std::function<Ret(Args..., void* user_data)> f, void* data)
				: func(std::move(f))
				, user_data(data)
			{
			}

			explicit operator bool() const { return static_cast<bool>(func); }
			Ret run(Args... args) const
			{
				if (func)
				{
					return func(args..., user_data);
				}

				return Ret();
			}
		};

		LvObj(lv_create_t initFunc, const std::string& name, LvObj& parent);
		LvObj(lv_create_t initFunc, const std::string& name, LvObj& parent, layout_t layout);
		LvObj(lv_create_t initFunc, const std::string& name)
			: LvObj(initFunc, name, lv_screen_active())
		{
		}

		// Delete copy constructor and assignment operator
		LvObj(const LvObj&) = delete;
		LvObj& operator=(const LvObj&) = delete;

		virtual ~LvObj();

		std::string_view getName() const;

		inline LvObj& getRoot() { return *this; }
		inline const LvObj& getRoot() const { return *this; }

		/**
		 * @return Get the base container for the view
		 */
		inline lv_obj_t* getRootPtr() const { return m_root; }
		operator lv_obj_t*() const { return getRootPtr(); }

		/* XML */
		static void registerWidgetXml();

		lv_obj_t* getScreen() const;
		lv_obj_t* getParent() const;
		lv_obj_t* getChild(int32_t id) const;
		uint32_t getChildCnt() const;
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

		void setUserData(void* user_data);
		void* getUserData() const;

		bool hasFlag(lv_obj_flag_t flag) const;
		bool hasState(lv_state_t state) const;
		bool hasStyleProp(lv_style_prop_t prop, lv_style_selector_t selector = LV_PART_MAIN) const;

		void setParent(LvObj& parent);
		void setLayoutStyle(lv_layout_t style);
		void setFlexGrow(uint8_t grow);
		void setFlexFlow(lv_flex_flow_t flow);
		void setFlexAlign(lv_flex_align_t main, lv_flex_align_t cross, lv_flex_align_t mid);
		void setGridDsc(const int32_t col_dsc[], const int32_t row_dsc[]);
		void setGridCell(lv_obj_t* obj,
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
		void setAlign(lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0);
		void setLocalStyleProp(lv_style_prop_t prop,
							   lv_style_value_t value,
							   lv_style_selector_t selector = LV_PART_MAIN);

		void updateLayout();
		bool refreshSelfSize();
		void invalidate();

		void setExtDrawSize(int32_t size);
		void setExtClickArea(int32_t size);
		void getClickArea(lv_area_t* area) const;

		void scrollBy(int32_t dx, int32_t dy, lv_anim_enable_t anim = LV_ANIM_OFF);
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

		lv_event_dsc_t* addEventCallback(lv_event_cb_t cb, lv_event_code_t code, void* userData);
		bool removeEvent(size_t index);
		uint32_t removeEventCallback(lv_event_cb_t cb);
		uint32_t removeEventCallbackWithUserData(lv_event_cb_t cb, void* userData);
		uint32_t getEventCount();
		lv_result_t sendEvent(lv_event_code_t code, void* param = nullptr);

		void moveToFront();
		void moveToBack();
		void moveToIndex(size_t index);
		void clearChildren();

		void setVisible(bool display, bool move_to_front = false)
		{
			display ? show(move_to_front) : hide(move_to_front);
		}
		virtual void show(bool move_to_front = false);
		virtual void hide(bool move_to_back = false);
		bool isVisible();

		virtual bool back();

	  protected:
		LvObj(lv_create_t initFunc, const std::string& name, lv_obj_t* parent);

		virtual void onShow() {}
		virtual void onHide() {}
		virtual void refresh() {}

	  private:
		lv_obj_t* m_root;
		std::string m_name;
	};
} // namespace UI

#define UI_LOCK()                                                                                                      \
	LOG_VERBOSE("UI_LOCK requested by thread {}", Log::GetThreadId());                                                 \
	auto uiLock = ScopedLock(mutexUi);
