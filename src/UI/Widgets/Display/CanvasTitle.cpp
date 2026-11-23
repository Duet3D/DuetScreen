/*
 * CanvasTitle.cpp
 */

#include "CanvasTitle.h"
#include "Debug.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Xml/Runtime/Translations.h"

namespace UI::Xml::CanvasTitle
{
	using namespace WidgetSupport;

	namespace
	{
		/** `bind_title` observer: its user data is the canvas */
		void titleObserverCb(lv_observer_t* observer, lv_subject_t* subject)
		{
			static_cast<Canvas*>(lv_observer_get_user_data(observer))->setTitle(lv_subject_get_string(subject));
		}

		/** `bind_detail` observer: its user data is the canvas */
		void detailObserverCb(lv_observer_t* observer, lv_subject_t* subject)
		{
			static_cast<Canvas*>(lv_observer_get_user_data(observer))->setDetail(lv_subject_get_string(subject));
		}
	} // namespace

	std::vector<WidgetAttribute> attributes()
	{
		return {{"title", "The title above the area, if there is no `title_tag`"},
				{"title_tag", "Translation key for the title", AttributeKind::Translation},
				{"bind_title", "Follow a text subject for the title", AttributeKind::Subject},
				{"title_style", "A theme style for the title, e.g. `text_emphasis`", AttributeKind::Style},
				{"bind_detail", "Follow a text subject shown after the title", AttributeKind::Subject}};
	}

	void apply(lv_xml_parser_state_t* state, const char** attrs, Canvas& canvas, std::string_view widget)
	{
		lv_obj_t* root = canvas.getRootPtr();
		if (const char* tag = lv_xml_get_value_of(attrs, "title_tag"))
		{
			canvas.setTitle(Translations::translate(tag));
		}
		else if (const char* title = lv_xml_get_value_of(attrs, "title"))
		{
			canvas.setTitle(title);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_title"))
		{
			lv_subject_add_observer_obj(subject, titleObserverCb, root, &canvas);
		}
		if (const char* styleName = lv_xml_get_value_of(attrs, "title_style"))
		{
			if (lv_xml_style_t* style = lv_xml_get_style_by_name(&state->scope, styleName))
			{
				canvas.getTitle().addStyle(lv_xml_style_get_lv_style(style));
			}
			else
			{
				LOG_WARN("{:s} title_style=\"{:s}\": no such style", widget, styleName);
			}
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_detail"))
		{
			lv_subject_add_observer_obj(subject, detailObserverCb, root, &canvas);
		}
	}
} // namespace UI::Xml::CanvasTitle
