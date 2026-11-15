/*
 * DsDialogFooter.cpp
 */

#include "DsDialogFooter.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/View/Actions.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <set>

namespace UI::Xml::DsDialogFooter
{
	using namespace WidgetSupport;

	namespace
	{
		/** Every footer */
		std::set<lv_obj_t*>& footers()
		{
			static std::set<lv_obj_t*> s_footers;
			return s_footers;
		}

		/** A footer is deleted: forget it */
		void deletedCb(lv_event_t* e)
		{
			footers().erase(lv_event_get_target_obj(e));
		}

		/** `<ds_dialog_footer>` create callback: a row of buttons, the main one last */
		void* xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
		{
			lv_obj_t* content = parentOf(state);
			lv_obj_t* footer = lv_obj_create(content);
			lv_obj_remove_style_all(footer);
			lv_obj_add_style(footer, Themes::getComponentStyles().dialog_footer, LV_PART_MAIN);
			lv_obj_remove_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
			// Shown under the content rather than in it, its actions still look outwards from it
			Actions::setOwner(footer, content);
			footers().insert(footer);
			lv_obj_add_event_cb(footer, deletedCb, LV_EVENT_DELETE, nullptr);
			return footer;
		}

		/** `<ds_dialog_footer>` apply callback: lv_obj attributes */
		void xmlApply(lv_xml_parser_state_t* state, const char** attrs)
		{
			lv_xml_obj_apply(state, attrs);
		}
	} // namespace

	void registerXml()
	{
		Widgets::add(
			{.name = "ds_dialog_footer",
			 .extends = "lv_obj",
			 .description =
				 "A dialog step's own buttons, written last in a `<ds_dialog>` or a prompt: a row, right-aligned, its "
				 "main button last (style it `action_btn`). While the step shows they stay at the bottom of the card, "
				 "below its content, which scrolls on its own. Styled `dialog_footer`.",
			 .example = R"(<ds_dialog_footer>
	<ds_button name="cancel" translation_tag="tr.common.cancel">
		<style name="dialog_button"/>
		<action trigger="clicked" name="actions.dialog.close"/>
	</ds_button>
	<ds_button name="ok" translation_tag="tr.common.ok">
		<style name="dialog_button"/>
		<style name="action_btn"/>
		<action trigger="clicked" name="actions.files.confirm"/>
	</ds_button>
</ds_dialog_footer>)"},
			xmlCreate,
			xmlApply);
	}

	lv_obj_t* in(lv_obj_t* content)
	{
		const uint32_t count = content != nullptr ? lv_obj_get_child_count(content) : 0;
		for (uint32_t i = 0; i < count; i++)
		{
			lv_obj_t* child = lv_obj_get_child(content, static_cast<int32_t>(i));
			if (footers().contains(child))
			{
				return child;
			}
		}
		return nullptr;
	}
} // namespace UI::Xml::DsDialogFooter
