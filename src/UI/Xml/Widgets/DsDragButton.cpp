/*
 * DsDragButton.cpp
 */

#include "DsDragButton.h"
#include "UI/Xml/View/Actions.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "i18n/i18n.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsDragButton::DsDragButton(const std::string& name, lv_obj_t* parent, std::string_view text)
		: DraggableButton(name, parent, text)
	{
		m_hint.setSize(LV_PCT(100), LV_PCT(100));
		m_hint.setRadius(LV_PCT(70));
		m_hint.setAngleStart(200);
		m_hint.setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);
		m_hint.setAlign(LV_ALIGN_CENTER, 0, 0);
		m_hint.hide();

		lv_obj_add_event_cb(getRootPtr(), pressCb, LV_EVENT_PRESSED, this);
		lv_obj_add_event_cb(getRootPtr(), pressCb, LV_EVENT_RELEASED, this);
		setDragCallback(
			[](float fraction, void* self)
			{
				lv_obj_send_event(
					static_cast<DsDragButton*>(self)->getRootPtr(), Actions::valueConfirmedEvent(), &fraction);
			},
			this);
	}

	void DsDragButton::registerXml()
	{
		Widgets::add(
			{.name = "ds_drag_button",
			 .extends = "lv_obj",
			 .description = "A button that must be dragged, not just tapped, to act, such as the E-stop. When let go "
							"it raises `value_confirmed` with how far it was dragged, from 0 (a tap) to 1 (all the "
							"way, when it shows its `user_1` state).",
			 .example = R"(<ds_drag_button name="estop" translation_tag="estop.label" icon="estop.png"
				 hint_tag="estop.prompt" width="80" height="80">
	<style name="estop"/>
	<action trigger="value_confirmed" name="estop.dragged"/>
</ds_drag_button>)",
			 .attributes = {{"text", "The text shown, if there is no `translation_tag`"},
							{"translation_tag", "Translation key for the text", AttributeKind::Translation},
							{"icon", "Icon file from the icon set, e.g. \"estop.png\""},
							{"hint", "Shown curved around the edge until it is held, if there is no `hint_tag`"},
							{"hint_tag", "Translation key for the hint", AttributeKind::Translation}}},
			xmlCreate,
			xmlApply);
	}

	void* DsDragButton::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		const char* tag = lv_xml_get_value_of(attrs, "translation_tag");
		const std::string text = tag ? _(tag) : attrOr(attrs, "text", "");
		auto* button = new DsDragButton(attrOr(attrs, "name", "drag_button"), parentOf(state), text);
		deleteWith(button->getRootPtr(), button);
		return button->getRootPtr();
	}

	void DsDragButton::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		auto& button = static_cast<DsDragButton&>(*LvObj::fromPtr(itemOf(state)));
		if (const char* icon = lv_xml_get_value_of(attrs, "icon"))
		{
			button.setIcon(icon);
		}
		const char* hintTag = lv_xml_get_value_of(attrs, "hint_tag");
		const char* hint = lv_xml_get_value_of(attrs, "hint");
		if (hintTag != nullptr || hint != nullptr)
		{
			button.m_hint.setText(hintTag ? _(hintTag) : std::string(hint));
			button.m_hint.show();
			button.m_hasHint = true;
		}
	}

	void DsDragButton::pressCb(lv_event_t* e)
	{
		auto* button = static_cast<DsDragButton*>(lv_event_get_user_data(e));
		if (!button->m_hasHint)
		{
			return;
		}
		button->m_hint.setFlag(LV_OBJ_FLAG_HIDDEN, lv_event_get_code(e) == LV_EVENT_PRESSED);
	}
} // namespace UI::Xml
