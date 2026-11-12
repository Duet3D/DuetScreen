/*
 * LvObj.cpp
 *
 *  Created on: 2025-06-10
 *      Author: Andy Everitt
 */

#include "LvObj.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	void lv_timer_delete_safe(lv_timer_t* timer)
	{
		if (timer && lv_is_initialized())
		{
			lv_timer_delete(timer);
		}
	}

	static lv_coord_t getPct(lv_coord_t value, lv_coord_t base)
	{
		return (value * 100 + base / 2) / base;
	}

	LvObj::LvObj(lv_create_t initFunc, const std::string& name, LvObj& parent)
		: LvObj(initFunc, name, parent.getRootPtr())
	{
	}

	LvObj::LvObj(lv_create_t initFunc, const std::string& name, lv_obj_t* parent)
	{
		ZoneScoped;
		UI_LOCK();
		parent = parent ? parent : lv_screen_active();
		m_root = initFunc(parent);

		lv_obj_set_name(m_root, name.c_str());
		lv_obj_set_user_data(m_root, this);
#if DEBUG
		std::string fullName = name;
		while (parent != nullptr)
		{
			const char* parentName = lv_obj_get_name(parent);
			parent = lv_obj_get_parent(parent);
			if (parentName == nullptr || parentName[0] == '\0')
			{
				continue;
			}
			fullName = fmt::format("{:s}.{:s}", parentName, fullName);
		}
		m_name = std::move(fullName);
#else
		{
			m_name = name;
		}
#endif

		LOG_DBG("Creating LvObj '{:s}' ({:p})", getName(), static_cast<const void*>(m_root));
		lv_obj_null_on_delete(&m_root);
	}

	LvObj::LvObj(lv_create_t initFunc, const std::string& name, LvObj& parent, layout_t layout)
		: LvObj(initFunc, name, parent)
	{
		ZoneScoped;
		setPos(LV_PCT(layout.x), LV_PCT(layout.y));
		setSize(layout.w == LV_SIZE_CONTENT ? LV_SIZE_CONTENT : LV_PCT(layout.w),
				layout.h == LV_SIZE_CONTENT ? LV_SIZE_CONTENT : LV_PCT(layout.h));
	}

	LvObj* LvObj::fromPtr(lv_obj_t* obj)
	{
		ZoneScoped;
		assert(obj != nullptr && "LvObj::fromPtr: obj is null");

		void* user_data = lv_obj_get_user_data(obj);
		if (user_data == nullptr)
			return nullptr;
		return static_cast<LvObj*>(user_data);
	}

	LvObj::LvObj(BorrowTag, lv_obj_t* existing)
		: m_root(existing)
		, m_name(existing && lv_obj_get_name(existing) ? lv_obj_get_name(existing) : "")
	{
	}

	LvObjRef::LvObjRef(lv_obj_t* obj)
		: LvObj(BorrowTag{}, obj)
	{
	}

	LvObjRef::~LvObjRef()
	{
		m_root = nullptr; // ~LvObj deletes the root otherwise
	}

	LvObj::~LvObj()
	{
		ZoneScoped;
		UI_LOCK();
		if (getRootPtr() != nullptr)
		{
			LOG_VERBOSE("Deleting obj '{:s}' ({:p})", getName(), static_cast<const void*>(getRootPtr()));
			lv_obj_t* obj = m_root;
			m_root = nullptr;
			lv_obj_delete(obj);
		}
	}

	std::string_view LvObj::getName() const
	{
		return m_name;
	}

	lv_display_t* LvObj::getDisplayPtr() const
	{
		UI_LOCK();
		return lv_obj_get_display(getRootPtr());
	}

	lv_obj_t* LvObj::getScreenPtr() const
	{
		UI_LOCK();
		return lv_obj_get_screen(getRootPtr());
	}

	LvObj* LvObj::getParent() const
	{
		return LvObj::fromPtr(getParentPtr());
	}

	lv_obj_t* LvObj::getParentPtr() const
	{
		UI_LOCK();
		return lv_obj_get_parent(getRootPtr());
	}

	LvObj* LvObj::getChild(int32_t id) const
	{
		UI_LOCK();
		lv_obj_t* child = getChildPtr(id);
		return LvObj::fromPtr(child);
	}

	lv_obj_t* LvObj::getChildPtr(int32_t id) const
	{
		UI_LOCK();
		return lv_obj_get_child(getRootPtr(), id);
	}

	/**
	 * @brief Recursively search children for a child with the given name. The input `name` will be split by the
	 * character `.` to allow searching through nested children.
	 * @param name Name of the child to search for, or a path of names separated by `.`.
	 * @return Pointer to the found LvObj, or nullptr if not found.
	 */
	LvObj* LvObj::getChildByName(std::string_view path) const
	{
		ZoneScoped;
		UI_LOCK();

		// Depth-first search helper with a single children loop.
		// Track most specific failure for accurate error reporting.
		const LvObj* failNode = this;
		std::string_view failSeg{};
		size_t failDepth = 0;

		auto dfs = [&](auto&& self, const LvObj* node, std::string_view remaining, size_t depth) -> LvObj*
		{
			const size_t dot = remaining.find('.');
			const std::string_view seg = remaining.substr(0, dot);
			const bool has_rest = dot != std::string_view::npos;
			const std::string_view rest = has_rest ? remaining.substr(dot + 1) : std::string_view{};

			for (size_t i = 0, n = node->getChildCount(); i < n; ++i)
			{
				const LvObj* child = node->getChild(static_cast<int32_t>(i));
				if (child == nullptr)
					continue;
				char name_buf[64];
				lv_obj_get_name_resolved(child->getRootPtr(), name_buf, sizeof(name_buf));

				// If this child matches the current segment, either return it or continue with the rest.
				if (seg == name_buf)
				{
					if (!has_rest)
					{
						return const_cast<LvObj*>(child);
					}
					if (LvObj* deeper = self(self, child, rest, depth + 1))
					{
						return deeper;
					}
				}
			}

			// Update failure for the current node if nothing found at this level.

			if (depth >= failDepth)
			{
				failDepth = depth;
				failNode = node;
				failSeg = seg;
			}
			return nullptr;
		};

		if (LvObj* result = dfs(dfs, this, path, 0))
		{
			return result;
		}

		// Log a single error if not found using the most specific failure context.
		char parent_buf[64]{};
		lv_obj_get_name_resolved(failNode->getRootPtr(), parent_buf, sizeof(parent_buf));
		/**
		 * For some reason, using string_view in the formatting causes an array-bounds error when
		 * compiling with -O2/-O3, but only on some computers?
		 *
		 * Also think it is only an issue when gtest is linked in?
		 *
		 * If anyone knows why, please tell me!
		 */
		LOG_ERROR("LvObj::getChildByName: Could not find child '{}' in '{}'", std::string(failSeg), parent_buf);
		return nullptr;
	}

	uint32_t LvObj::getChildCount() const
	{
		UI_LOCK();
		return lv_obj_get_child_count(getRootPtr());
	}

	layout_t LvObj::getLayout() const
	{
		ZoneScoped;
		UI_LOCK();
		layout_t layout;
		layout.x = lv_obj_get_x(getRootPtr());
		layout.y = lv_obj_get_y(getRootPtr());
		layout.w = lv_obj_get_width(getRootPtr());
		layout.h = lv_obj_get_height(getRootPtr());

		return layout;
	}

	layout_t LvObj::getLayoutPct() const
	{
		ZoneScoped;
		UI_LOCK();
		layout_t layout = getLayout();
		lv_coord_t wParent = lv_obj_get_width(getParentPtr());
		lv_coord_t hParent = lv_obj_get_height(getParentPtr());

		layout.x = getPct(layout.x, wParent);
		layout.y = getPct(layout.y, hParent);
		layout.w = getPct(layout.w, wParent);
		layout.h = getPct(layout.h, hParent);

		return layout;
	}

	lv_area_t LvObj::getCoords() const
	{
		ZoneScoped;
		UI_LOCK();
		lv_area_t area;
		lv_obj_get_coords(getRootPtr(), &area);
		return area;
	}

	lv_coord_t LvObj::getX() const
	{
		UI_LOCK();
		return lv_obj_get_x(getRootPtr());
	}

	lv_coord_t LvObj::getY() const
	{
		UI_LOCK();
		return lv_obj_get_y(getRootPtr());
	}

	lv_coord_t LvObj::getX2() const
	{
		UI_LOCK();
		return lv_obj_get_x2(getRootPtr());
	}

	lv_coord_t LvObj::getY2() const
	{
		UI_LOCK();
		return lv_obj_get_y2(getRootPtr());
	}

	lv_coord_t LvObj::getWidth() const
	{
		UI_LOCK();
		return lv_obj_get_width(getRootPtr());
	}

	lv_coord_t LvObj::getHeight() const
	{
		UI_LOCK();
		return lv_obj_get_height(getRootPtr());
	}

	lv_coord_t LvObj::getContentWidth() const
	{
		UI_LOCK();
		return lv_obj_get_content_width(getRootPtr());
	}

	lv_coord_t LvObj::getContentHeight() const
	{
		UI_LOCK();
		return lv_obj_get_content_height(getRootPtr());
	}

	lv_coord_t LvObj::getSelfWidth() const
	{
		UI_LOCK();
		return lv_obj_get_self_width(getRootPtr());
	}

	lv_coord_t LvObj::getSelfHeight() const
	{
		UI_LOCK();
		return lv_obj_get_self_height(getRootPtr());
	}

	lv_style_value_t LvObj::getStyleProp(lv_style_prop_t prop, lv_part_t part) const
	{
		UI_LOCK();
		return lv_obj_get_style_prop(getRootPtr(), part, prop);
	}

	void LvObj::setUserData(void* user_data)
	{
		UI_LOCK();
		m_userData = user_data;
	}

	void* LvObj::getUserData() const
	{
		UI_LOCK();
		return m_userData;
	}

	void LvObj::setParent(LvObj& parent)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_parent(getRootPtr(), parent.getRootPtr());
	}

	void LvObj::setLayoutStyle(lv_layout_t style)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_layout(getRootPtr(), style);
	}

	void LvObj::setFlexGrow(uint8_t grow)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_flex_grow(getRootPtr(), grow);
	}

	void LvObj::setFlexFlow(lv_flex_flow_t flow)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_flex_flow(getRootPtr(), flow);
	}

	void LvObj::setFlexAlign(lv_flex_align_t main, lv_flex_align_t cross, lv_flex_align_t mid)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_flex_align(getRootPtr(), main, cross, mid);
	}

	void LvObj::setGridDsc(const std::span<const int32_t> col_dsc, const std::span<const int32_t> row_dsc)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_grid_dsc_array(getRootPtr(), col_dsc.data(), row_dsc.data());
	}

	void LvObj::setGridCell(LvObj& obj,
							lv_grid_align_t x_align,
							int32_t col_pos,
							int32_t col_span,
							lv_grid_align_t y_align,
							int32_t row_pos,
							int32_t row_span)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_grid_cell(obj.getRootPtr(), x_align, col_pos, col_span, y_align, row_pos, row_span);
	}

	void LvObj::setLayout(layout_t layout)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_pos(getRootPtr(), lv_pct(layout.x), lv_pct(layout.y));
		lv_obj_set_size(getRootPtr(), lv_pct(layout.w), lv_pct(layout.h));
	}

	void LvObj::setWidth(lv_coord_t width)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_width(getRootPtr(), width);
	}

	void LvObj::setHeight(lv_coord_t height)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_height(getRootPtr(), height);
	}

	void LvObj::setSize(lv_coord_t width, lv_coord_t height)
	{
		ZoneScoped;
		UI_LOCK();
		setWidth(width);
		setHeight(height);
	}

	void LvObj::setMinWidth(lv_coord_t width, lv_style_selector_t selector)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_style_min_width(getRootPtr(), width, selector);
	}

	void LvObj::setMinHeight(lv_coord_t height, lv_style_selector_t selector)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_style_min_height(getRootPtr(), height, selector);
	}

	void LvObj::setMaxWidth(lv_coord_t width, lv_style_selector_t selector)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_style_max_width(getRootPtr(), width, selector);
	}

	void LvObj::setMaxHeight(lv_coord_t height, lv_style_selector_t selector)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_style_max_height(getRootPtr(), height, selector);
	}

	void LvObj::setX(lv_coord_t x)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_x(getRootPtr(), x);
	}

	void LvObj::setY(lv_coord_t y)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_y(getRootPtr(), y);
	}

	void LvObj::setPos(lv_coord_t x, lv_coord_t y)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_pos(getRootPtr(), x, y);
	}

	static void lv_obj_set_flag(lv_obj_t* obj, lv_obj_flag_t flag, bool enable, bool recursive)
	{
		ZoneScoped;
		lv_obj_set_flag(obj, flag, enable);
		if (recursive)
		{
			for (uint32_t i = 0; i < lv_obj_get_child_count(obj); i++)
			{
				lv_obj_t* child = lv_obj_get_child(obj, static_cast<int32_t>(i));
				if (child == nullptr)
				{
					continue;
				}
				lv_obj_set_flag(child, flag, enable, true);
			}
		}
	}

	void LvObj::setFlag(lv_obj_flag_t flag, bool enable, bool recursive)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_flag(getRootPtr(), flag, enable, recursive);
	}

	bool LvObj::hasFlag(lv_obj_flag_t flag) const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_has_flag(getRootPtr(), flag);
	}

	static void lv_obj_set_state(lv_obj_t* obj, lv_state_t state, bool enable, bool recursive)
	{
		ZoneScoped;
		lv_obj_set_state(obj, state, enable);
		if (recursive)
		{
			for (size_t i = 0; i < lv_obj_get_child_count(obj); i++)
			{
				lv_obj_t* child = lv_obj_get_child(obj, static_cast<int32_t>(i));
				if (child == nullptr)
				{
					continue;
				}
				lv_obj_set_state(child, state, enable, true);
			}
		}
	}

	void LvObj::setState(lv_state_t state, bool enable, bool recursive)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_state(getRootPtr(), state, enable, recursive);
	}

	bool LvObj::hasState(lv_state_t state) const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_has_state(getRootPtr(), state);
	}

	bool LvObj::hasStyle(const lv_style_t* style) const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_has_style(getRootPtr(), style);
	}

	bool LvObj::hasStyleProp(lv_style_prop_t prop, lv_style_selector_t selector) const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_has_style_prop(getRootPtr(), selector, prop);
	}

	void LvObj::setAlign(lv_align_t align)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_align(getRootPtr(), align);
	}

	void LvObj::setAlign(lv_align_t align, lv_coord_t x, lv_coord_t y)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_align(getRootPtr(), align, x, y);
	}

	void LvObj::setAlignTo(LvObj& target, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_align_to(getRootPtr(), target.getRootPtr(), align, x_ofs, y_ofs);
	}

	void LvObj::setLocalStyleProp(lv_style_prop_t prop, lv_style_value_t value, lv_style_selector_t selector)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_local_style_prop(getRootPtr(), prop, value, selector);
	}

	bool LvObj::removeLocalStyleProp(lv_style_prop_t prop, lv_style_selector_t selector)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_remove_local_style_prop(getRootPtr(), prop, selector);
	}

	void LvObj::updateLayout()
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_update_layout(getRootPtr());
	}

	bool LvObj::refreshSelfSize()
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_refresh_self_size(getRootPtr());
	}

	void LvObj::invalidate()
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_invalidate(getRootPtr());
	}

	void LvObj::markLayoutDirty()
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_mark_layout_as_dirty(getRootPtr());
	}

	static void __obj_set_ext_draw_size_cb(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		int32_t s = (int32_t)(intptr_t)lv_event_get_user_data(e);
		lv_event_code_t code = lv_event_get_code(e);
		if (code == LV_EVENT_REFR_EXT_DRAW_SIZE)
		{
			lv_event_set_ext_draw_size(e, s);
		}
		else
		{
			LOG_FATAL_THROW("Unexpected event code: {:d}", (int32_t)code);
		}
	}

	void LvObj::setExtDrawSize(int32_t size)
	{
		ZoneScoped;
		UI_LOCK();
		setFlag(LV_OBJ_FLAG_OVERFLOW_VISIBLE, true);
		removeEventCallback(__obj_set_ext_draw_size_cb);
		addEventCallback(__obj_set_ext_draw_size_cb, LV_EVENT_REFR_EXT_DRAW_SIZE, (void*)(intptr_t)size);
	}

	void LvObj::setExtClickArea(int32_t size)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_ext_click_area(getRootPtr(), size);
	}

	void LvObj::getClickArea(lv_area_t* area) const
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_get_click_area(getRootPtr(), area);
	}

	void LvObj::setScrollbarMode(lv_scrollbar_mode_t mode)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_scrollbar_mode(getRootPtr(), mode);
	}

	lv_scrollbar_mode_t LvObj::getScrollbarMode() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_get_scrollbar_mode(getRootPtr());
	}

	void LvObj::scrollBy(int32_t dx, int32_t dy, lv_anim_enable_t anim)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_scroll_by(getRootPtr(), dx, dy, anim);
	}

	void LvObj::scrollByBounded(int32_t dx, int32_t dy, lv_anim_enable_t anim)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_scroll_by_bounded(getRootPtr(), dx, dy, anim);
	}

	void LvObj::scrollToX(lv_coord_t x, lv_anim_enable_t anim)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_scroll_to_x(getRootPtr(), x, anim);
	}

	void LvObj::scrollToY(lv_coord_t y, lv_anim_enable_t anim)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_scroll_to_y(getRootPtr(), y, anim);
	}

	void LvObj::setScrollDir(lv_dir_t dir)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_set_scroll_dir(getRootPtr(), dir);
	}

	lv_coord_t LvObj::getScrollLeft() const
	{
		UI_LOCK();
		return lv_obj_get_scroll_left(getRootPtr());
	}

	lv_coord_t LvObj::getScrollRight() const
	{
		UI_LOCK();
		return lv_obj_get_scroll_right(getRootPtr());
	}

	lv_coord_t LvObj::getScrollTop() const
	{
		UI_LOCK();
		return lv_obj_get_scroll_top(getRootPtr());
	}

	lv_coord_t LvObj::getScrollBottom() const
	{
		UI_LOCK();
		return lv_obj_get_scroll_bottom(getRootPtr());
	}

	void LvObj::addStyle(const lv_style_t* style, const lv_style_selector_t selector, bool recursive)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_add_style(getRootPtr(), style, selector, recursive);
	}

	void LvObj::removeStyle(const lv_style_t* style, const lv_style_selector_t selector, bool recursive)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_remove_style(getRootPtr(), style, selector, recursive);
	}

	void LvObj::setStylePad(lv_coord_t pad, lv_style_selector_t selector, Padding type)
	{
		ZoneScoped;
		UI_LOCK();
		switch (type)
		{
		case Padding::ALL:
			lv_obj_set_style_pad_all(getRootPtr(), pad, selector);
			break;
		case Padding::LEFT:
			lv_obj_set_style_pad_left(getRootPtr(), pad, selector);
			break;
		case Padding::RIGHT:
			lv_obj_set_style_pad_right(getRootPtr(), pad, selector);
			break;
		case Padding::TOP:
			lv_obj_set_style_pad_top(getRootPtr(), pad, selector);
			break;
		case Padding::BOTTOM:
			lv_obj_set_style_pad_bottom(getRootPtr(), pad, selector);
			break;
		case Padding::COLUMN:
			lv_obj_set_style_pad_column(getRootPtr(), pad, selector);
			break;
		case Padding::ROW:
			lv_obj_set_style_pad_row(getRootPtr(), pad, selector);
			break;
		case Padding::HORIZONTAL:
			lv_obj_set_style_pad_hor(getRootPtr(), pad, selector);
			break;
		case Padding::VERTICAL:
			lv_obj_set_style_pad_ver(getRootPtr(), pad, selector);
			break;
		default:
			LOG_WARN("Unknown padding type");
			break;
		}
	}

	void LvObj::setStyleBgColor(lv_color_t color, lv_style_selector_t selector)
	{
		UI_LOCK();
		lv_obj_set_style_bg_color(getRootPtr(), color, selector);
	}

	void LvObj::setStyleBgOpa(lv_opa_t opa, lv_style_selector_t selector)
	{
		UI_LOCK();
		lv_obj_set_style_bg_opa(getRootPtr(), opa, selector);
	}

	void LvObj::setStyleRecolor(lv_color_t color, lv_style_selector_t selector)
	{
		UI_LOCK();
		lv_obj_set_style_recolor(getRootPtr(), color, selector);
	}

	void LvObj::setStyleRecolorOpa(lv_opa_t opa, lv_style_selector_t selector)
	{
		UI_LOCK();
		lv_obj_set_style_recolor_opa(getRootPtr(), opa, selector);
	}

	void LvObj::setStyleTextAlign(lv_text_align_t align, lv_style_selector_t selector)
	{
		UI_LOCK();
		lv_obj_set_style_text_align(getRootPtr(), align, selector);
	}

	void LvObj::setStyleFont(const lv_font_t* font, lv_style_selector_t selector)
	{
		UI_LOCK();
		lv_obj_set_style_text_font(getRootPtr(), font, selector);
	}

	lv_event_dsc_t* LvObj::addEventCallback(lv_event_cb_t cb, lv_event_code_t code, void* userData)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_add_event_cb(getRootPtr(), cb, code, userData);
	}

	void LvObj::addEventCallback(const std::function<void(lv_event_t*)>& cb, lv_event_code_t code)
	{
		ZoneScoped;
		UI_LOCK();
		if (!cb)
		{
			return;
		}
		if (m_eventCallbacks.empty())
		{
			lv_obj_add_event_cb(getRootPtr(), genericEventCallback, LV_EVENT_ALL, this);
		}
		m_eventCallbacks.emplace_back(std::move(cb), code);
	}

	void LvObj::genericEventCallback(lv_event_t* e)
	{
		ZoneScoped;
		LvObj* obj = static_cast<LvObj*>(lv_event_get_user_data(e));
		if (obj == nullptr)
			return;

		lv_event_code_t code = lv_event_get_code(e);
		// Iterate by index to avoid iterator invalidation if vector grows
		for (size_t i = 0; i < obj->m_eventCallbacks.size(); ++i)
		{
			const auto cbCode = obj->m_eventCallbacks[i].code;
			if (cbCode == LV_EVENT_ALL || cbCode == code)
			{
				/**
				 * warning: if the callback adds or removes other callbacks while iterating, it may invalidate the
				 * iterator. A reference is used for efficiency.
				 */
				const auto& cb = obj->m_eventCallbacks[i].cb;
				if (cb)
				{
					ZoneScoped;
					[[maybe_unused]] auto tag = lv_event_code_get_name(code);
					ZoneName(tag, strlen(tag));
					std::invoke(cb, e);
				}
			}
		}
	}

	bool LvObj::removeEvent(uint32_t index)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_remove_event(getRootPtr(), index);
	}

	uint32_t LvObj::removeEventCallback(lv_event_cb_t cb)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_remove_event_cb(getRootPtr(), cb);
	}

	uint32_t LvObj::removeEventCallbackWithUserData(lv_event_cb_t cb, void* userData)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_remove_event_cb_with_user_data(getRootPtr(), cb, userData);
	}

	uint32_t LvObj::getEventCount()
	{
		UI_LOCK();
		return lv_obj_get_event_count(getRootPtr());
	}

	lv_result_t LvObj::sendEvent(lv_event_code_t code, void* param)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_send_event(getRootPtr(), code, param);
	}

	void LvObj::moveToFront()
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_move_foreground(getRootPtr());
	}

	void LvObj::moveToBack()
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_move_background(getRootPtr());
	}

	void LvObj::moveToIndex(int32_t index)
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_move_to_index(getRootPtr(), index);
	}

	void UI::LvObj::clearChildren()
	{
		ZoneScoped;
		UI_LOCK();
		while (lv_obj_get_child_cnt(getRootPtr()) > 0)
		{
			lv_obj_t* child = lv_obj_get_child(getRootPtr(), 0);
			lv_obj_delete(child);
		}
	}

	void LvObj::setVisible(bool display, bool move_to_front)
	{
		ZoneScoped;
		if (display == isVisible())
			return;
		display ? show(move_to_front) : hide(move_to_front);
	}

	/**
	 * @brief Shows the view.
	 *
	 * @note This function calls the `onShow()` virtual method before showing the view.
	 */
	void LvObj::show(bool move_to_front)
	{
		ZoneScoped;
		{
			UI_LOCK();
			if (getRootPtr() == nullptr || m_showing)
			{
				return;
			}

			if (!m_initialized)
			{
				m_initialized = 1;
				onInit();
			}

			if (!hasFlag(LV_OBJ_FLAG_HIDDEN))
			{
				LOG_VERBOSE("'{:s}' is already visible", getName());
				/* Don't return */
			}

			LOG_DBG("Showing '{:s}'", getName());
			if (move_to_front)
			{
				moveToFront();
			}
		}

		setFlag(LV_OBJ_FLAG_HIDDEN, false);
		showInner();
		lv_display_trigger_activity(NULL); // Reset screensaver timer
	}

	void LvObj::showInner()
	{
		ZoneScoped;
		/* Stop recursive call to same LvObj */
		if (m_showing)
		{
			return;
		}

		if (!m_initialized)
		{
			m_initialized = 1;
			onInit();
		}

		m_showing = 1;
#if LV_NESTED_SHOW_HIDE
		iterateChildren(
			[](size_t /* index */, LvObj& child)
			{
				if (child.isVisible())
					child.showInner();
			});
#endif

		onShow();

		m_showing = 0;
	}

	/**
	 * @brief Hides the view.
	 *
	 * @note This function calls the `onHide()` virtual method before hiding the view.
	 */
	void LvObj::hide(bool move_to_back)
	{
		ZoneScoped;
		UI_LOCK();
		if (getRootPtr() == nullptr || m_hidding)
		{
			return;
		}
		if (hasFlag(LV_OBJ_FLAG_HIDDEN))
		{
			LOG_VERBOSE("'{:s}' is already hidden", getName());
			return;
		}

		LOG_DBG("Hiding '{:s}'", getName());
		if (move_to_back)
		{
			moveToBack();
		}

		setFlag(LV_OBJ_FLAG_HIDDEN, true);
		hideInner();
	}

	void LvObj::hideInner()
	{
		ZoneScoped;
		if (m_hidding)
		{
			return;
		}

		m_hidding = 1;
#if LV_NESTED_SHOW_HIDE
		iterateChildren(
			[](size_t, LvObj& child)
			{
				if (child.isVisible())
				{
					/* want to run `deactivate` on any children with presenters, and onHide(), but also want the child
					 * to be visible again when obj is shown */
					child.hideInner();
				}
			});
#endif

		onHide();

		m_hidding = 0;
	}

	bool LvObj::isVisible()
	{
		return !hasFlag(LV_OBJ_FLAG_HIDDEN);
	}

	/**
	 * @brief Handle a back button event
	 * @return true if the view handled the back event
	 */
	bool LvObj::back()
	{
		return false;
	}
} // namespace UI
