/*
 * test_value_selector.cpp
 *
 * A value with presets (assets/ui/components/value_selector.xml, src/UI/Xml/View/ValueSelector.h).
 */

#include "UI/Xml/View/ValueSelector.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "UI/Xml/View/XmlView.h"
#include "helix_xml.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <memory>

/** Shows one selector, kept nowhere */
class SelectorPresenter : public UI::Xml::XmlPresenter
{
  public:
	SelectorPresenter()
		: XmlPresenter("test_selector")
	{
		m_selector.setTitle("Selector");
		for (auto& [name, description, handler] : UI::Xml::ValueSelector::actions({&m_selector}))
		{
			action(name, description, handler);
		}
	}

	void fields(UI::Xml::FieldVisitor& f) override { m_selector.fields(f); }

  private:
	UI::Xml::ValueSelector m_selector{{.id = "test", .presets = {0.1f, 0.5f, 1.0f, 5.0f, 10.0f}}}; // The selector
};

class TestValueSelector : public UiTestSuite
{
  protected:
	TestValueSelector()
	{
		// A value_selector filling a view, given its props as a screen would
		lv_xml_register_component_from_data("test_value_selector", R"(
			<component>
				<view extends="lv_obj" style_pad_all="0">
					<value_selector name="selector" selector="test" title="test.title" value="test.value"
									choices="test.choices" width="100%" height="100%"/>
				</view>
			</component>)");
	}
};

TEST_F(TestValueSelector, Basic)
{
	UI::Xml::XmlView view("selector", screen, "test_value_selector", std::make_shared<SelectorPresenter>());
	view.setSize(400, 200);
	view.show();

	EXPECT_EQUAL_SCREENSHOT("value_selector/basic.png");
}
