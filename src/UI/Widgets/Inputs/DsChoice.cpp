/*
 * DsChoice.cpp
 */

#include "DsChoice.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/Translations.h"
#include "UI/Xml/View/Actions.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <charconv>

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsChoice::DsChoice(const std::string& name, lv_obj_t* parent)
		: LvObj(lv_button_create, name, parent)
		, m_label("label", getRoot())
		, m_chevron("chevron", getRoot())
	{
		setUserData(this);
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		// The label takes the room the chevron leaves, but never less than its text, so a choice sized to its
		// content fits the text on one line
		m_label.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_label.setFlexGrow(1);
		m_label.setMinWidth(LV_SIZE_CONTENT);
		m_label.setLongMode(LV_LABEL_LONG_MODE_DOTS);
		m_label.setText("");

		m_chevron.setText(LV_SYMBOL_RIGHT);
		m_chevron.addStyle(Themes::getLvglStyles().text_muted);

		lv_obj_add_event_cb(getRootPtr(), clickedCb, LV_EVENT_CLICKED, this);
	}

	void DsChoice::registerXml()
	{
		Widgets::add(
			{.name = "ds_choice",
			 .extends = "lv_obj",
			 .description =
				 "A button showing the option chosen, with a chevron. Tapping it opens the choice dialog, with the "
				 "option chosen marked; the option picked reaches a `value_confirmed` action as its index "
				 "(ActionArgs::value()). It shows what `bind_selected` says is chosen, so the action sets that "
				 "subject; without `bind_selected` it shows the option picked.",
			 .example = R"(<ds_choice title_tag="tr.settings.language" bind_options="presenter.languages"
		   bind_selected="presenter.language" width="300">
	<action trigger="value_confirmed" name="presenter.settings.language"/>
</ds_choice>)",
			 .attributes = {{"bind_options",
							 "Follow a text subject holding the options, one per line",
							 AttributeKind::Subject},
							{"bind_selected",
							 "Follow an int subject for the index of the option chosen; -1 for none",
							 AttributeKind::Subject},
							{"bind_text",
							 "Follow a text subject shown instead of the option chosen, e.g. the filament loaded",
							 AttributeKind::Subject},
							{"title", "The choice dialog's title, if there is no `title_tag`"},
							{"title_tag", "Translation key for the choice dialog's title", AttributeKind::Translation}},
			 .exampleSubjects = {{"presenter.languages", "string", "English (UK)\nDeutsch\nFrançais"},
								 {"presenter.language", "int", "0"}}},
			xmlCreate,
			xmlApply);
	}

	void* DsChoice::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* choice = new DsChoice(attrOr(attrs, "name", "choice"), parentOf(state));
		deleteWith(choice->getRootPtr(), choice);
		return choice->getRootPtr();
	}

	void DsChoice::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto* choice = static_cast<DsChoice*>(LvObj::fromPtr(root));
		if (const char* title = lv_xml_get_value_of(attrs, "title"))
		{
			choice->m_title = title;
		}
		if (const char* tag = lv_xml_get_value_of(attrs, "title_tag"))
		{
			choice->m_titleTag = tag;
		}

		// The options come first, so the option chosen can be found among them
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_options"))
		{
			lv_subject_add_observer_obj(subject, optionsObserverCb, root, choice);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_selected"))
		{
			choice->m_chosenBound = true;
			lv_subject_add_observer_obj(subject, selectedObserverCb, root, choice);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_text"))
		{
			choice->m_textBound = true;
			lv_subject_add_observer_obj(subject, textObserverCb, root, choice);
		}
	}

	void DsChoice::optionsObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& choice = *static_cast<DsChoice*>(lv_observer_get_user_data(observer));
		choice.m_options.clear();
		const std::string_view options = lv_subject_get_string(subject);
		size_t start = 0;
		while (start < options.size())
		{
			const size_t end = std::min(options.find('\n', start), options.size());
			choice.m_options.emplace_back(options.substr(start, end - start));
			start = end + 1;
		}
		choice.showChosen();
	}

	void DsChoice::selectedObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& choice = *static_cast<DsChoice*>(lv_observer_get_user_data(observer));
		choice.m_chosen = lv_subject_get_int(subject);
		choice.showChosen();
	}

	void DsChoice::textObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& choice = *static_cast<DsChoice*>(lv_observer_get_user_data(observer));
		choice.m_text = lv_subject_get_string(subject);
		choice.showChosen();
	}

	void DsChoice::clickedCb(lv_event_t* e)
	{
		static_cast<DsChoice*>(lv_event_get_user_data(e))->ask();
	}

	void DsChoice::showChosen()
	{
		if (m_textBound)
		{
			m_label.setText(m_text);
		}
		else if (m_chosen >= 0 && static_cast<size_t>(m_chosen) < m_options.size())
		{
			m_label.setText(m_options[static_cast<size_t>(m_chosen)]);
		}
		else
		{
			m_label.setText("");
		}
	}

	void DsChoice::ask()
	{
		Prompts::ChoiceRequest request{.title = m_titleTag.empty() ? m_title : Translations::translate(m_titleTag),
									   .chosen = std::to_string(m_chosen),
									   .onChoose =
										   [this](const std::string& value)
									   {
										   size_t index = 0;
										   const auto [ptr, ec] =
											   std::from_chars(value.data(), value.data() + value.size(), index);
										   if (ec == std::errc() && index < m_options.size())
										   {
											   pick(index);
										   }
									   },
									   .asker = getRootPtr()};
		request.choices.reserve(m_options.size());
		for (size_t i = 0; i < m_options.size(); i++)
		{
			request.choices.push_back({.value = std::to_string(i), .title = m_options[i]});
		}
		Prompts::askChoice(std::move(request));
	}

	void DsChoice::pick(size_t index)
	{
		if (!m_chosenBound)
		{
			m_chosen = static_cast<int32_t>(index);
			showChosen();
		}
		float value = static_cast<float>(index);
		lv_obj_send_event(getRootPtr(), Actions::valueConfirmedEvent(), &value);
	}
} // namespace UI::Xml
