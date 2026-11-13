/*
 * DsHueSlider.cpp
 */

#include "DsHueSlider.h"
#include "UI/Styles/Color.h"
#include "WidgetSupport.h"
#include "Widgets.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	/** The hues offered */
	static constexpr float MAX_HUE = 360.0f;

	/** `bind_lightness` and `bind_chroma` hold their values times this */
	static constexpr float THOUSANDTHS = 1000.0f;

	DsHueSlider::DsHueSlider(const std::string& name, lv_obj_t* parent)
		: DsSlider(name, parent)
	{
		setRangeQuietly(0.0f, MAX_HUE);

		// The hues lie under the bar, which shows them through its transparent track and indicator
		auto& bar = getLvSlider();
		LvObj& row = *LvObj::fromPtr(lv_obj_get_parent(bar.getRootPtr()));
		m_hues = std::make_unique<Canvas>("hues", row);
		lv_obj_move_to_index(m_hues->getRootPtr(), 0);
		m_hues->setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);
		m_hues->setResolution(static_cast<uint32_t>(MAX_HUE), 1u);
		m_hues->showTitle(false);
		m_hues->showXScale(false);
		m_hues->showYScale(false);
		m_hues->setStylePad(0);
		m_hues->setStylePad(0, LV_PART_MAIN, Padding::ROW);
		m_hues->setStylePad(0, LV_PART_MAIN, Padding::COLUMN);
		bar.setStyleBgOpa(LV_OPA_0);
		bar.setStyleBgOpa(LV_OPA_0, LV_PART_INDICATOR);
		bar.addEventCallback(barEventCb, LV_EVENT_SIZE_CHANGED, this);
		bar.addEventCallback(barEventCb, LV_EVENT_STYLE_CHANGED, this);
		render();
	}

	void DsHueSlider::registerXml()
	{
		Widgets::add(
			{.name = "ds_hue_slider",
			 .extends = "ds_slider",
			 .description =
				 "A `<ds_slider>` for a hue, 0 to 360, whose track shows every hue at a lightness and chroma, so the "
				 "slider is a colour picker, e.g. for a theme's primary colour. `min` and `max` default to 0 and 360.",
			 .example =
				 R"(<ds_hue_slider bind_value="primary_hue" bind_lightness="primary.lightness" bind_chroma="primary.chroma"/>)",
			 .attributes =
				 {{"bind_lightness",
				   "Follow an int subject for the OKLCH lightness the hues are shown at, in thousandths (0.65 is 650)"},
				  {"bind_chroma", "Follow an int subject for the OKLCH chroma the hues are shown at, in thousandths"}}},
			xmlCreate,
			xmlApply);
	}

	void DsHueSlider::setColour(float lightness, float chroma)
	{
		m_lightness = lightness;
		m_chroma = chroma;
		render();
	}

	void* DsHueSlider::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* slider = new DsHueSlider(attrOr(attrs, "name", "hue_slider"), parentOf(state));
		deleteWith(slider->getRootPtr(), slider);
		return slider->getRootPtr();
	}

	void DsHueSlider::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		DsSlider::xmlApply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& slider = static_cast<DsHueSlider&>(*LvObj::fromPtr(root));
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_lightness"))
		{
			lv_subject_add_observer_obj(subject, lightnessObserverCb, root, &slider);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_chroma"))
		{
			lv_subject_add_observer_obj(subject, chromaObserverCb, root, &slider);
		}
	}

	void DsHueSlider::lightnessObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& slider = *static_cast<DsHueSlider*>(lv_observer_get_user_data(observer));
		slider.setColour(static_cast<float>(lv_subject_get_int(subject)) / THOUSANDTHS, slider.m_chroma);
	}

	void DsHueSlider::chromaObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& slider = *static_cast<DsHueSlider*>(lv_observer_get_user_data(observer));
		slider.setColour(slider.m_lightness, static_cast<float>(lv_subject_get_int(subject)) / THOUSANDTHS);
	}

	void DsHueSlider::barEventCb(lv_event_t* e)
	{
		auto& slider = *static_cast<DsHueSlider*>(lv_event_get_user_data(e));
		auto& bar = slider.getLvSlider();
		slider.m_hues->setSize(bar.getWidth(), bar.getHeight());
		slider.m_hues->setAlignTo(bar, LV_ALIGN_TOP_LEFT, 0, 0);
		slider.m_hues->getCanvas().setLocalStyleProp(LV_STYLE_RADIUS, bar.getStyleProp(LV_STYLE_RADIUS));
	}

	void DsHueSlider::render()
	{
		uint32_t width = 0;
		uint32_t height = 0;
		m_hues->getResolution(width, height);
		for (uint32_t x = 0; x < width; x++)
		{
			m_hues->drawPx(
				x, 0, static_cast<lv_color_t>(Color(m_lightness, m_chroma, static_cast<float>(x))), LV_OPA_COVER);
		}
		m_hues->getCanvas().invalidate();
	}
} // namespace UI::Xml
