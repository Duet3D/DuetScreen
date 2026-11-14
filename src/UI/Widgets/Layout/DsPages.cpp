/*
 * DsPages.cpp
 */

#include "DsPages.h"
#include "UI/Core/Navigation.h"
#include "UI/Lvgl/Transitions.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Runtime/Translations.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** The size of the back button's icon, before the title of the page it goes back to */
		constexpr lv_coord_t BACK_ICON_SIZE = 24;

		/** The gap between the header and the page, and between the back button and the title */
		constexpr lv_coord_t HEADER_GAP = 10;

		/** Every stack there is, so one around a widget can be found from the widget */
		std::vector<DsPages*> s_stacks;
	} // namespace

	DsPages::DsPages(const std::string& name, lv_obj_t* parent)
		: LvContainer(name, parent)
	{
		setStylePad(0);
		setStylePad(HEADER_GAP, LV_PART_MAIN, Padding::ROW);
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_header.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_header.setStylePad(0);
		m_header.setStylePad(HEADER_GAP, LV_PART_MAIN, Padding::COLUMN);
		m_header.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_header.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_header.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
		m_header.hide();

		m_back.setIcon("back.png");
		m_back.setIconBeforeText(true, BACK_ICON_SIZE);
		m_back.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_back.addClickedCallback([this](lv_event_t*) { back(); });

		m_title.addStyle(Themes::getLvglStyles().text_header);
		m_title.setFlexGrow(1);
		m_title.setLongMode(LV_LABEL_LONG_MODE_DOTS);

		m_body.setWidth(LV_PCT(100));
		m_body.setFlexGrow(1);
		m_body.setStylePad(0);
		m_body.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		lv_obj_add_event_cb(getRootPtr(), languageChangedCb, LV_EVENT_TRANSLATION_LANGUAGE_CHANGED, this);
		s_stacks.push_back(this);
		addBackLayer(this);
	}

	DsPages::~DsPages()
	{
		removeBackLayer(this);
		std::erase(s_stacks, this);
	}

	void DsPages::registerXml()
	{
		Widgets::add({.name = "ds_pages",
					  .extends = "lv_obj",
					  .description =
						  "Pages that share one place, the first shown to start with. Each is a `<ds_pages-page>` "
						  "inside it. `actions.navigation.open_page` from inside the stack slides the page it names "
						  "in, under a header with the page's title and a button back to the page it came from; that "
						  "button, or the back key while the stack is on screen, slides back.",
					  .example = R"(<ds_pages width="100%" height="200">
	<ds_pages-page name="display" title="Display">
		<ds_button text="Colours">
			<action trigger="clicked" name="actions.navigation.open_page" arg="colours"/>
		</ds_button>
	</ds_pages-page>
	<ds_pages-page name="colours" title="Colours">
		<lv_label text="The colours"/>
	</ds_pages-page>
</ds_pages>)"},
					 xmlCreate,
					 xmlApply);
		Widgets::add({.name = "ds_pages-page",
					  .extends = "lv_obj",
					  .description = "One page of a `<ds_pages>`, written inside it, opened by its `name`. Its "
									 "content is the page; the lv_obj attributes apply to the page.",
					  .attributes = {{"title", "The page's title, shown over it, if there is no `title_tag`"},
									 {"title_tag",
									  "Translation key for the page's title; follows language changes",
									  AttributeKind::Translation}}},
					 xmlCreatePage,
					 xmlApplyPage);
	}

	bool DsPages::open(std::string_view name)
	{
		UI_LOCK();
		const size_t index = indexOf(name);
		if (index >= m_pages.size())
		{
			LOG_WARN("<ds_pages name=\"{:s}\"> has no page called \"{:s}\"", getName(), name);
			return false;
		}
		const size_t shown = m_shown.back();
		if (index == shown)
		{
			return true;
		}
		m_shown.push_back(index);
		slideTo(shown, index, 1);
		return true;
	}

	bool DsPages::back()
	{
		UI_LOCK();
		if (m_shown.size() < 2)
		{
			return false;
		}
		const size_t shown = m_shown.back();
		m_shown.pop_back();
		slideTo(shown, m_shown.back(), -1);
		return true;
	}

	std::string_view DsPages::shownPage() const
	{
		return m_shown.empty() ? std::string_view() : std::string_view(m_pages[m_shown.back()].name);
	}

	DsPages* DsPages::around(lv_obj_t* obj, std::string_view name)
	{
		for (; obj != nullptr; obj = lv_obj_get_parent(obj))
		{
			for (DsPages* stack : s_stacks)
			{
				if (stack->getRootPtr() == obj && stack->indexOf(name) < stack->m_pages.size())
				{
					return stack;
				}
			}
		}
		return nullptr;
	}

	void* DsPages::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* pages = new DsPages(attrOr(attrs, "name", "pages"), parentOf(state));
		deleteWith(pages->getRootPtr(), pages);
		return pages->getRootPtr();
	}

	void DsPages::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
	}

	void* DsPages::xmlCreatePage(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_obj_t* parent = parentOf(state);
		DsPages* pages = nullptr;
		for (DsPages* stack : s_stacks)
		{
			if (stack->getRootPtr() == parent)
			{
				pages = stack;
			}
		}
		if (pages == nullptr)
		{
			LOG_WARN("<ds_pages-page> must be inside <ds_pages>");
			return lv_obj_create(parent);
		}

		const char* tag = lv_xml_get_value_of(attrs, "title_tag");
		std::string title = tag != nullptr ? Translations::translate(tag) : attrOr(attrs, "title", "");
		return pages->addPage(attrOr(attrs, "name", "page"), std::move(title), tag != nullptr ? tag : "").getRootPtr();
	}

	void DsPages::xmlApplyPage(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
	}

	void DsPages::languageChangedCb(lv_event_t* e)
	{
		auto* pages = static_cast<DsPages*>(lv_event_get_user_data(e));
		for (Page& page : pages->m_pages)
		{
			if (!page.titleTag.empty())
			{
				page.title = Translations::translate(page.titleTag);
			}
		}
		pages->showHeader();
	}

	LvContainer& DsPages::addPage(std::string name, std::string title, std::string titleTag)
	{
		auto content = std::make_unique<LvContainer>(name, m_body);
		content->setSize(LV_PCT(100), LV_PCT(100));
		content->setStylePad(0);
		content->setFlag(LV_OBJ_FLAG_HIDDEN, !m_pages.empty());
		LvContainer& added = *content;
		m_pages.push_back({.content = std::move(content),
						   .name = std::move(name),
						   .title = std::move(title),
						   .titleTag = std::move(titleTag)});
		if (m_shown.empty())
		{
			m_shown.push_back(0);
		}
		return added;
	}

	size_t DsPages::indexOf(std::string_view name) const
	{
		const auto it =
			std::find_if(m_pages.begin(), m_pages.end(), [name](const Page& page) { return page.name == name; });
		return static_cast<size_t>(it - m_pages.begin());
	}

	void DsPages::slideTo(size_t from, size_t to, int32_t direction)
	{
		showHeader();
		updateLayout();
		Transitions::slide(
			m_pages[from].content.get(), *m_pages[to].content, direction, m_body.getWidth(), isShownOnScreen());
	}

	void DsPages::showHeader()
	{
		if (m_shown.size() < 2)
		{
			m_header.hide();
			return;
		}
		m_back.setText(m_pages[m_shown[m_shown.size() - 2]].title);
		m_title.setText(m_pages[m_shown.back()].title);
		m_header.show();
	}
} // namespace UI::Xml
