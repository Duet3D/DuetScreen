/*
 * FactorsPresenter.cpp
 */

#include "FactorsPresenter.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Data/ToolSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"
#include <cmath>
#include <fmt/format.h>

namespace UI
{
	namespace
	{
		constexpr float MAX_PERCENT = 10000.0f; // The most that can be typed for either factor

		/** `percent` rounded to a whole number */
		uint32_t whole(float percent)
		{
			return static_cast<uint32_t>(std::lround(percent));
		}

		/** The shared dialogs' presenter, or nullptr if there is none */
		FactorsPresenter* sharedFactors()
		{
			auto* view = findView<Xml::XmlView>("factors");
			auto* presenter = view != nullptr ? dynamic_cast<FactorsPresenter*>(view->getPresenter()) : nullptr;
			if (presenter == nullptr)
			{
				LOG_WARN("There are no factor dialogs (a view called 'factors' showing factors.xml)");
			}
			return presenter;
		}
	} // namespace

	FactorsPresenter::FactorsPresenter()
		: XmlPresenter("factors")
		, m_speed({.id = "speed_dialog",
				   .title = _("fine_tune.speed_factor"),
				   .min = 1.0f,
				   .max = MAX_PERCENT,
				   .sliderMin = 1,
				   .sliderMax = 150,
				   .rangeFollowsValue = true,
				   .onSet =
					   [](size_t, float percent)
				   {
					   if (whole(percent) != whole(100 * OM::Move::GetSpeedFactor()))
					   {
						   Xml::Services::sendGcode(fmt::format("M220 S{:d}", whole(percent)));
					   }
				   }})
		, m_extrusion({.id = "extrusion_dialog",
					   .title = _("fine_tune.extruder_header"),
					   .min = 0.0f,
					   .max = MAX_PERCENT,
					   .sliderMin = 0,
					   .sliderMax = 200,
					   .padOnOpen = true,
					   .onSet = [this](size_t index, float percent)
					   {
						   MODEL_LOCK();
						   auto extruder = extruderAt(index);
						   if (extruder != nullptr && whole(percent) != whole(100 * extruder->factor))
						   {
							   Xml::Services::sendGcode(
								   fmt::format("M221 D{:d} S{:d}", extruder->index, whole(percent)));
						   }
					   }})
	{
		for (auto& [name, description, handler] : FactorDialog::actions({&m_speed, &m_extrusion}))
		{
			action(name, description, handler);
		}
	}

	void FactorsPresenter::fields(Xml::FieldVisitor& f)
	{
		m_speed.fields(f);
		m_extrusion.fields(f);
	}

	void FactorsPresenter::registerActions()
	{
		Xml::Actions::addGlobal("factors.speed",
								"Open the dialog to change the speed factor",
								[](const Xml::ActionArgs&)
								{
									if (auto* factors = sharedFactors())
									{
										factors->openSpeed();
									}
								});
		Xml::Actions::addGlobal(
			"factors.extrusion",
			"Open the dialog to change extrusion factors: the tool's, from a row of a list of tools, else every "
			"extruder's",
			[](const Xml::ActionArgs& args)
			{
				auto* factors = sharedFactors();
				if (factors == nullptr)
				{
					return;
				}
				const auto* row = args.row<Xml::ToolRow>();
				factors->openExtrusion(row != nullptr ? std::optional(static_cast<size_t>(row->number.get()))
													  : std::nullopt);
			});
	}

	void FactorsPresenter::openSpeed()
	{
		showSpeed();
		m_speed.open();
	}

	void FactorsPresenter::openExtrusion(std::optional<size_t> tool)
	{
		m_tool = tool;
		showExtrusion();
		m_extrusion.open();
	}

	void FactorsPresenter::onInit()
	{
		registerEventListener<EventType::SpeedFactor>([this]() { showSpeed(); });
		registerEventListener<EventType::ExtruderData>([this]() { showExtrusion(); });
		registerEventListener<EventType::ToolData>([this]() { showExtrusion(); });
	}

	void FactorsPresenter::showSpeed()
	{
		const float percent = 100 * OM::Move::GetSpeedFactor();
		m_speed.show(1, [](size_t) { return std::string(); }, [percent](size_t) { return percent; });
	}

	void FactorsPresenter::showExtrusion()
	{
		MODEL_LOCK();
		m_extrusion.show(
			extruderCount(),
			[this](size_t i)
			{
				auto extruder = extruderAt(i);
				return extruder ? _("fine_tune.extruder", extruder->index) : std::string();
			},
			[this](size_t i)
			{
				auto extruder = extruderAt(i);
				return extruder ? 100.0f * extruder->factor : 100.0f;
			});
	}

	OM::Move::ExtruderAxisPtr FactorsPresenter::extruderAt(size_t index) const
	{
		if (!m_tool)
		{
			return OM::Move::GetExtruderAxisBySlot(index);
		}
		auto tool = OM::GetTool(*m_tool);
		return tool ? tool->GetExtruder(index) : nullptr;
	}

	size_t FactorsPresenter::extruderCount() const
	{
		if (!m_tool)
		{
			return OM::Move::GetExtruderAxisCount();
		}
		auto tool = OM::GetTool(*m_tool);
		return tool ? tool->GetExtruderCount() : 0;
	}
} // namespace UI
