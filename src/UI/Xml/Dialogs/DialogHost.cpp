/*
 * DialogHost.cpp
 */

#include "DialogHost.h"
#include "Debug.h"
#include "DialogHostPresenter.h"
#include "DsDialog.h"
#include "DsDialogFooter.h"
#include "PromptPresenters.h"
#include "UI/Lvgl/LvObj.h"
#include "UI/Lvgl/Transitions.h"
#include "UI/Widgets/Inputs/KeyboardRoom.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/View/XmlView.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <cstdlib>
#include <string_view>
#include <utility>

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** The component drawing the surface's chrome */
		constexpr const char* CHROME = "dialog_host";

		/** Every surface, in the order made */
		std::vector<DialogHost*>& hosts()
		{
			static std::vector<DialogHost*> s_hosts;
			return s_hosts;
		}

		/** Whether animations are on (the display settings) */
		bool animate()
		{
			return StorageHelper::getData(ID_UI_ANIMATIONS_ENABLED);
		}

		/** Whether `obj` is `ancestor` or inside it */
		bool isInside(const lv_obj_t* obj, const lv_obj_t* ancestor)
		{
			for (; obj != nullptr; obj = lv_obj_get_parent(obj))
			{
				if (obj == ancestor)
				{
					return true;
				}
			}
			return false;
		}

		/**
		 * A size written as XML writes one, in `room` pixels: a percentage of the room, pixels, or nothing for
		 * `content`. Never more than the room.
		 */
		std::optional<int32_t> pixelsOf(std::string_view size, int32_t room)
		{
			if (size.empty() || size == "content")
			{
				return std::nullopt;
			}
			const int32_t number = std::atoi(std::string(size).c_str());
			const int32_t pixels = size.ends_with('%') ? room * number / 100 : number;
			return std::clamp(pixels, 0, room);
		}

		/** The animation sliding a step's content: its horizontal offset */
		void translateXCb(void* obj, int32_t x)
		{
			lv_obj_set_style_translate_x(static_cast<lv_obj_t*>(obj), x, LV_PART_MAIN);
		}

		/** A slide ended: the content is in its place, with no offset left behind */
		void slideDoneCb(lv_anim_t* anim)
		{
			lv_obj_remove_local_style_prop(static_cast<lv_obj_t*>(anim->var), LV_STYLE_TRANSLATE_X, LV_PART_MAIN);
		}

		/** The animation fading the surface in */
		void opaCb(void* obj, int32_t opa)
		{
			lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(opa), LV_PART_MAIN);
		}

		/** The surface has faded in: no opacity left behind */
		void fadeDoneCb(lv_anim_t* anim)
		{
			lv_obj_remove_local_style_prop(static_cast<lv_obj_t*>(anim->var), LV_STYLE_OPA, LV_PART_MAIN);
		}

		/** `<ds_dialog_host>` create callback: the surface, deleted with its widget */
		void* xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
		{
			auto* host = new DialogHost(parentOf(state), true);
			deleteWith(host->root(), host);
			return host->root();
		}

		/** `<ds_dialog_host>` apply callback: lv_obj attributes, e.g. its place in a grid */
		void xmlApply(lv_xml_parser_state_t* state, const char** attrs)
		{
			lv_xml_obj_apply(state, attrs);
		}
	} // namespace

	DialogHost::DialogHost(lv_obj_t* parent, bool navigation)
		: m_parent(parent)
		, m_presenter(std::make_shared<DialogHostPresenter>())
		, m_navigation(navigation)
	{
		m_presenter->attach(this);
		m_chrome = std::make_unique<XmlView>(CHROME, parent, CHROME, m_presenter);
		lv_obj_t* root = m_chrome->getRootPtr();
		m_card = lv_obj_find_by_name(root, "card");
		m_body = lv_obj_find_by_name(root, "body");
		m_footer = lv_obj_find_by_name(root, "footer");
		if (m_card == nullptr || m_body == nullptr)
		{
			LOG_ERROR("The dialog surface's component '{:s}' needs a 'card' with a 'body' in it", CHROME);
		}
		m_chrome->show();
		lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
		m_parking = lv_obj_create(root);
		lv_obj_remove_style_all(m_parking);
		lv_obj_add_flag(m_parking, LV_OBJ_FLAG_HIDDEN);

		hosts().push_back(this);
		if (m_navigation)
		{
			setDialogLayer(this);
		}
	}

	DialogHost::~DialogHost()
	{
		// The chrome is deleted with this, so the contents of the steps that live elsewhere go home first; the steps
		// the surface owns go with it, before the chrome
		for (auto& entry : m_steps)
		{
			park(*entry.step);
		}
		for (auto& alert : m_alerts)
		{
			for (auto& entry : alert)
			{
				park(*entry.step);
			}
		}
		m_steps.clear();
		m_alerts.clear();
		m_retired.clear();
		m_top = nullptr;
		lv_async_call_cancel(deleteRetiredCb, this);
		lv_async_call_cancel(scrollToFocusedCb, this);

		std::erase(hosts(), this);
		if (m_navigation)
		{
			setDialogLayer(nullptr);
		}
		m_presenter->attach(nullptr);
	}

	lv_obj_t* DialogHost::root() const
	{
		return m_chrome->getRootPtr();
	}

	void DialogHost::registerXml()
	{
		Presenters::add<DialogHostPresenter>("dialog_host");
		Presenters::add<NumberPrompt>("dialog_number");
		Presenters::add<ChoicePrompt>("dialog_choice");
		Presenters::add<QuestionPrompt>("dialog_question");
		Widgets::add({.name = "ds_dialog_host",
					  .extends = "lv_obj",
					  .description =
						  "The dialog surface: one card over a dimmed backdrop that every `<ds_dialog>` and prompt on "
						  "the screen opens in, one step at a time. It fills where it is placed (the home screen puts "
						  "it over its main window), and is drawn by the component `dialog_host`. A screen without "
						  "one gets one covering it."},
					 xmlCreate,
					 xmlApply);
		DsDialog::registerXml();
		DsDialogFooter::registerXml();
	}

	DialogHost& DialogHost::of(lv_obj_t* obj)
	{
		lv_obj_t* screen = lv_obj_get_screen(obj);
		for (DialogHost* host : hosts())
		{
			if (lv_obj_get_screen(host->m_parent) == screen)
			{
				return *host;
			}
		}
		auto* host = new DialogHost(screen, false);
		deleteWith(host->root(), host);
		return *host;
	}

	DialogHost* DialogHost::holding(lv_obj_t* obj)
	{
		for (DialogHost* host : hosts())
		{
			if (host->m_card != nullptr && isInside(obj, host->m_card))
			{
				return host;
			}
		}
		return nullptr;
	}

	bool DialogHost::exists(const DialogHost* host)
	{
		return std::find(hosts().begin(), hosts().end(), host) != hosts().end();
	}

	DialogHost* DialogHost::showing(const DialogStep& step)
	{
		for (DialogHost* host : hosts())
		{
			if (host->has(step))
			{
				return host;
			}
		}
		return nullptr;
	}

	void DialogHost::open(DialogStep& step)
	{
		if (!has(step))
		{
			add({.step = &step});
		}
	}

	void DialogHost::open(std::unique_ptr<DialogStep> step)
	{
		DialogStep* raw = step.get();
		add({.step = raw, .owned = std::move(step)});
	}

	void DialogHost::close(DialogStep& step)
	{
		if (auto found = find(step))
		{
			auto& [stack, index] = *found;
			// Back to the step before, if this was on top of one; the stack may be gone once it is removed
			const bool back = m_top == &step && index > 0;
			remove(*stack, index, index + 1);
			update(back ? Motion::backward : Motion::none);
		}
	}

	void DialogHost::forget(DialogStep& step)
	{
		close(step);
	}

	void DialogHost::finish(DialogStep& step)
	{
		auto found = find(step);
		if (!found)
		{
			return;
		}
		auto& [stack, index] = *found;
		const bool back = m_top == &step && index > 0;
		Entry entry = std::move((*stack)[index]);
		stack->erase(stack->begin() + static_cast<std::ptrdiff_t>(index));
		park(step);
		if (m_top == &step)
		{
			m_top = nullptr;
		}
		std::erase_if(m_alerts, [](const std::vector<Entry>& alert) { return alert.empty(); });
		if (entry.owned)
		{
			m_retired.push_back(std::move(entry.owned));
			lv_async_call_cancel(deleteRetiredCb, this);
			lv_async_call(deleteRetiredCb, this);
		}
		update(back ? Motion::backward : Motion::none);
	}

	void DialogHost::deleteRetiredCb(void* host)
	{
		static_cast<DialogHost*>(host)->m_retired.clear();
	}

	bool DialogHost::has(const DialogStep& step) const
	{
		const auto in = [&step](const std::vector<Entry>& stack)
		{ return std::any_of(stack.begin(), stack.end(), [&step](const Entry& e) { return e.step == &step; }); };
		return in(m_steps) || std::any_of(m_alerts.begin(), m_alerts.end(), in);
	}

	lv_obj_t* DialogHost::shownContent() const
	{
		return m_top != nullptr ? m_top->content() : nullptr;
	}

	void DialogHost::closeShown()
	{
		auto& stack = shown();
		if (stack.empty() || anyBlocking(stack))
		{
			return;
		}
		remove(stack, 0, 0);
		update(Motion::none);
	}

	void DialogHost::goBack()
	{
		auto& stack = shown();
		if (stack.empty() || stack.back().step->blocking())
		{
			return;
		}
		if (stack.size() == 1)
		{
			closeShown();
			return;
		}
		remove(stack, stack.size() - 1, stack.size() - 1);
		update(Motion::backward);
	}

	void DialogHost::makeRoom(int32_t keyboardTop, lv_obj_t* focused)
	{
		// Handed on to another box, with the room made already, the card stays as it is: laying it out again would
		// lose where it is scrolled to
		const bool made = m_keyboardTop == keyboardTop;
		m_keyboardTop = keyboardTop;
		m_focused = focused;
		if (m_top != nullptr)
		{
			if (!made)
			{
				layout(*m_top);
			}
			// LVGL scrolls a box into view as it is focused, in the card as it was; the box is scrolled to once the
			// smaller card is laid out, which also replaces that scroll
			lv_async_call_cancel(scrollToFocusedCb, this);
			lv_async_call(scrollToFocusedCb, this);
		}
	}

	void DialogHost::scrollToFocusedCb(void* host)
	{
		auto* self = static_cast<DialogHost*>(host);
		if (self->m_focused != nullptr && lv_obj_is_valid(self->m_focused) && isInside(self->m_focused, self->m_card))
		{
			lv_obj_update_layout(self->m_card);
			lv_obj_scroll_to_view_recursive(self->m_focused, LV_ANIM_ON);
		}
	}

	void DialogHost::giveRoomBack()
	{
		lv_async_call_cancel(scrollToFocusedCb, this);
		m_keyboardTop.reset();
		m_focused = nullptr;
		if (m_top != nullptr)
		{
			layout(*m_top);
		}
	}

	void DialogHost::headerChanged()
	{
		if (m_top != nullptr)
		{
			layout(*m_top);
		}
	}

	bool DialogHost::back()
	{
		if (!isOpen())
		{
			return false;
		}
		// A step that must be answered keeps the back key too
		goBack();
		return true;
	}

	void DialogHost::closeAll()
	{
		if (m_steps.empty())
		{
			return;
		}
		remove(m_steps, 0, 0);
		update(Motion::none);
	}

	void DialogHost::closeNotAround(lv_obj_t* obj)
	{
		size_t keep = 0;
		for (size_t i = 0; i < m_steps.size(); i++)
		{
			if (isInside(obj, m_steps[i].step->content()))
			{
				keep = i + 1;
			}
		}
		if (keep < m_steps.size())
		{
			remove(m_steps, keep, keep);
			update(Motion::none);
		}
	}

	bool DialogHost::isOpen() const
	{
		return !m_steps.empty() || !m_alerts.empty();
	}

	std::vector<DialogHost::Entry>& DialogHost::shown()
	{
		return m_alerts.empty() ? m_steps : m_alerts.front();
	}

	const std::vector<DialogHost::Entry>& DialogHost::shown() const
	{
		return m_alerts.empty() ? m_steps : m_alerts.front();
	}

	std::optional<std::pair<std::vector<DialogHost::Entry>*, size_t>> DialogHost::find(const DialogStep& step)
	{
		const auto in = [&step](std::vector<Entry>& stack) -> std::optional<size_t>
		{
			for (size_t i = 0; i < stack.size(); i++)
			{
				if (stack[i].step == &step)
				{
					return i;
				}
			}
			return std::nullopt;
		};
		if (auto index = in(m_steps))
		{
			return std::pair{&m_steps, *index};
		}
		for (auto& alert : m_alerts)
		{
			if (auto index = in(alert))
			{
				return std::pair{&alert, *index};
			}
		}
		return std::nullopt;
	}

	void DialogHost::add(Entry entry)
	{
		if (entry.step->alert())
		{
			// An alert waits its turn behind those already shown
			std::vector<Entry> alert;
			alert.push_back(std::move(entry));
			m_alerts.push_back(std::move(alert));
			update(Motion::none);
			return;
		}
		m_steps.push_back(std::move(entry));
		update(m_steps.size() > 1 ? Motion::forward : Motion::none);
	}

	void DialogHost::remove(std::vector<Entry>& stack, size_t index, size_t dismissFrom)
	{
		// Take every step off first: telling one it was dismissed may open or close others
		std::vector<std::pair<Entry, bool>> removed;
		while (stack.size() > index)
		{
			const bool dismiss = stack.size() - 1 >= dismissFrom;
			removed.emplace_back(std::move(stack.back()), dismiss);
			stack.pop_back();
			park(*removed.back().first.step);
			if (m_top == removed.back().first.step)
			{
				m_top = nullptr;
			}
		}
		std::erase_if(m_alerts, [](const std::vector<Entry>& alert) { return alert.empty(); });
		for (auto& [entry, dismiss] : removed)
		{
			if (dismiss)
			{
				entry.step->dismissed();
			}
			// A step the surface owns may be in the middle of handling an event (its own Cancel button)
			if (entry.owned)
			{
				m_retired.push_back(std::move(entry.owned));
				lv_async_call_cancel(deleteRetiredCb, this);
				lv_async_call(deleteRetiredCb, this);
			}
		}
	}

	bool DialogHost::anyBlocking(const std::vector<Entry>& stack)
	{
		return std::any_of(stack.begin(), stack.end(), [](const Entry& entry) { return entry.step->blocking(); });
	}

	void DialogHost::update(Motion motion)
	{
		lv_obj_t* root = m_chrome->getRootPtr();
		const auto& stack = shown();
		DialogStep* next = stack.empty() ? nullptr : stack.back().step;
		// Only a card already showing slides to its next step; one opening fades in
		const bool wasHidden = lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN);
		if (next != nullptr && wasHidden)
		{
			// Shown before the step is laid out: LVGL lays out nothing hidden, so the card couldn't be measured
			lv_obj_remove_flag(root, LV_OBJ_FLAG_HIDDEN);
			lv_obj_move_foreground(root);
		}

		// The header first: whether it shows is part of the room the step has
		updateHeader();

		if (next != m_top)
		{
			if (m_top != nullptr)
			{
				park(*m_top);
			}
			m_top = next;
			if (next != nullptr && m_body != nullptr)
			{
				lv_obj_t* content = next->content();
				lv_obj_set_parent(content, m_body);
				showFooter(content);
				// Views in it come alive, as they would on a page shown
				LvObj::setVisible(content, true);
				lv_obj_set_pos(content, 0, 0);
				layout(*next);
				if (!wasHidden && motion != Motion::none)
				{
					lv_obj_update_layout(m_card);
					const int32_t width = lv_obj_get_width(m_body);
					slideIn(content, motion == Motion::forward ? width : -width);
					if (m_shownFooter != nullptr)
					{
						slideIn(m_shownFooter, motion == Motion::forward ? width : -width);
					}
				}
			}
		}

		if (next == nullptr)
		{
			lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
		}
		else if (wasHidden)
		{
			if (animate())
			{
				lv_anim_t anim;
				lv_anim_init(&anim);
				lv_anim_set_var(&anim, root);
				lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
				lv_anim_set_exec_cb(&anim, opaCb);
				lv_anim_set_duration(&anim, Transitions::animFastDurationMs());
				lv_anim_set_completed_cb(&anim, fadeDoneCb);
				lv_anim_start(&anim);
			}
		}
		notifyNavigation();
	}

	void DialogHost::park(DialogStep& step)
	{
		lv_obj_t* content = step.content();
		lv_obj_t* home = step.home();
		if (content == nullptr)
		{
			return;
		}
		// An edit in it ends as it goes out of sight
		KeyboardRoom::putAwayFrom(content);
		returnFooter(content);
		lv_anim_delete(content, translateXCb);
		lv_obj_remove_local_style_prop(content, LV_STYLE_TRANSLATE_X, LV_PART_MAIN);
		if (home != nullptr && lv_obj_get_parent(content) != home)
		{
			lv_obj_set_parent(content, home);
		}
		LvObj::setVisible(content, false);
	}

	void DialogHost::showFooter(lv_obj_t* content)
	{
		lv_obj_t* footer = m_footer != nullptr ? DsDialogFooter::in(content) : nullptr;
		if (footer == nullptr)
		{
			if (m_footer != nullptr)
			{
				lv_obj_add_flag(m_footer, LV_OBJ_FLAG_HIDDEN);
			}
			return;
		}
		m_shownFooter = footer;
		m_footerIndex = static_cast<uint32_t>(lv_obj_get_index(footer));
		lv_obj_set_parent(footer, m_footer);
		lv_obj_add_event_cb(footer, footerDeletedCb, LV_EVENT_DELETE, this);
		lv_obj_remove_flag(m_footer, LV_OBJ_FLAG_HIDDEN);
	}

	void DialogHost::returnFooter(lv_obj_t* content)
	{
		if (m_shownFooter == nullptr)
		{
			return;
		}
		lv_obj_t* footer = std::exchange(m_shownFooter, nullptr);
		lv_obj_remove_event_cb_with_user_data(footer, footerDeletedCb, this);
		lv_anim_delete(footer, translateXCb);
		lv_obj_remove_local_style_prop(footer, LV_STYLE_TRANSLATE_X, LV_PART_MAIN);
		lv_obj_set_parent(footer, content);
		lv_obj_move_to_index(footer, static_cast<int32_t>(m_footerIndex));
		if (m_footer != nullptr)
		{
			lv_obj_add_flag(m_footer, LV_OBJ_FLAG_HIDDEN);
		}
	}

	void DialogHost::footerDeletedCb(lv_event_t* e)
	{
		auto* host = static_cast<DialogHost*>(lv_event_get_user_data(e));
		if (host->m_shownFooter == lv_event_get_target_obj(e))
		{
			host->m_shownFooter = nullptr;
		}
	}

	void DialogHost::layout(DialogStep& step)
	{
		if (m_card == nullptr || m_body == nullptr)
		{
			return;
		}
		lv_obj_t* root = m_chrome->getRootPtr();
		lv_obj_t* backdrop = lv_obj_find_by_name(root, "backdrop");
		const int32_t margin = backdrop != nullptr ? lv_obj_get_style_pad_top(backdrop, LV_PART_MAIN) : 0;

		// The room: the surface less its margin, and above the keyboard while it is up
		lv_obj_update_layout(root);
		lv_area_t area;
		lv_obj_get_coords(root, &area);
		const int32_t bottom = m_keyboardTop ? std::min(area.y2, *m_keyboardTop) : area.y2;
		const int32_t roomWidth = std::max<int32_t>(0, lv_area_get_width(&area) - 2 * margin);
		const int32_t roomHeight = std::max<int32_t>(0, bottom - area.y1 + 1 - 2 * margin);

		const DialogStep::Size size = step.size();
		lv_obj_set_width(m_card, pixelsOf(size.width, roomWidth).value_or(roomWidth));
		if (m_keyboardTop)
		{
			lv_obj_align(m_card, LV_ALIGN_TOP_MID, 0, margin);
		}
		else
		{
			lv_obj_align(m_card, LV_ALIGN_CENTER, 0, 0);
		}

		lv_obj_t* content = step.content();
		if (const auto height = pixelsOf(size.height, roomHeight))
		{
			// A card of a set height: the step fills what the header leaves, and scrolls itself
			lv_obj_set_height(m_card, std::min(*height, roomHeight));
			lv_obj_remove_local_style_prop(m_card, LV_STYLE_MAX_HEIGHT, LV_PART_MAIN);
			lv_obj_set_flex_grow(m_body, 1);
			lv_obj_set_height(m_body, 0);
			lv_obj_remove_local_style_prop(m_body, LV_STYLE_MAX_HEIGHT, LV_PART_MAIN);
			lv_obj_remove_flag(m_body, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_scroll_to(m_body, 0, 0, LV_ANIM_OFF);
			lv_obj_set_size(content, lv_pct(100), lv_pct(100));
			return;
		}

		// A card as tall as the step: the body scrolls once it reaches the room left under the header
		lv_obj_set_height(m_card, LV_SIZE_CONTENT);
		lv_obj_set_flex_grow(m_body, 0);
		lv_obj_set_height(m_body, LV_SIZE_CONTENT);
		lv_obj_add_flag(m_body, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_scroll_dir(m_body, LV_DIR_VER);
		lv_obj_set_size(content, lv_pct(100), LV_SIZE_CONTENT);

		// What the card adds around the body (its header, padding and border), measured with neither held back
		lv_obj_remove_local_style_prop(m_card, LV_STYLE_MAX_HEIGHT, LV_PART_MAIN);
		lv_obj_remove_local_style_prop(m_body, LV_STYLE_MAX_HEIGHT, LV_PART_MAIN);
		lv_obj_update_layout(m_card);
		const int32_t chrome = lv_obj_get_height(m_card) - lv_obj_get_height(m_body);
		lv_obj_set_style_max_height(m_body, std::max<int32_t>(0, roomHeight - chrome), LV_PART_MAIN);
		lv_obj_set_style_max_height(m_card, roomHeight, LV_PART_MAIN);
	}

	void DialogHost::slideIn(lv_obj_t* content, int32_t fromX)
	{
		if (!animate() || fromX == 0)
		{
			return;
		}
		lv_anim_t anim;
		lv_anim_init(&anim);
		lv_anim_set_var(&anim, content);
		lv_anim_set_values(&anim, fromX, 0);
		lv_anim_set_exec_cb(&anim, translateXCb);
		lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
		lv_anim_set_duration(&anim, Transitions::animDurationMs());
		lv_anim_set_completed_cb(&anim, slideDoneCb);
		lv_anim_start(&anim);
	}

	void DialogHost::updateHeader()
	{
		const auto& stack = shown();
		if (stack.empty())
		{
			m_presenter->show(nullptr, nullptr, false, false);
			return;
		}
		const DialogStep* top = stack.back().step;
		lv_subject_t* from = stack.size() > 1 ? stack[stack.size() - 2].step->title() : nullptr;
		m_presenter->show(stack.back().step->title(), from, stack.size() > 1 && !top->blocking(), !anyBlocking(stack));
	}

	void DialogHost::notifyNavigation() const
	{
		if (m_navigation)
		{
			dialogsChanged();
		}
	}
} // namespace UI::Xml
