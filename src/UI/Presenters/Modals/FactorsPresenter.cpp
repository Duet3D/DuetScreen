/*
 * FactorsPresenter.cpp
 */

#include "FactorsPresenter.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Data/MoveSource.h"
#include "UI/Xml/Data/PrinterModel.h"
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

	} // namespace

	FactorsPresenter* FactorsPresenter::shared()
	{
		auto* view = findView<Xml::XmlView>("factors");
		auto* presenter = view != nullptr ? dynamic_cast<FactorsPresenter*>(view->getPresenter()) : nullptr;
		if (presenter == nullptr)
		{
			LOG_WARN("There are no factor dialogs (a view called 'factors' showing factors.xml)");
		}
		return presenter;
	}

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
					   if (whole(percent) != whole(100 * Xml::MoveSource::speedFactor()))
					   {
						   Xml::Services::sendGcode(fmt::format("M220 S{:d}", whole(percent)));
					   }
				   }})
		, m_extrusion(
			  {.id = "extrusion_dialog",
			   .title = _("fine_tune.extruder_header"),
			   .min = 0.0f,
			   .max = MAX_PERCENT,
			   .sliderMin = 0,
			   .sliderMax = 200,
			   .padOnOpen = true,
			   .onSet = [this](size_t index, float percent)
			   {
				   std::optional<size_t> extruder;
				   float factor = 0;
				   {
					   MODEL_LOCK();
					   const auto numbers = extruders();
					   if (index < numbers.size())
					   {
						   extruder = numbers[index];
						   factor = static_cast<float>(
							   Xml::PrinterModel::root()["move"]["extruders"][numbers[index]]["factor"].number());
					   }
				   }
				   if (extruder && whole(percent) != whole(100 * factor))
				   {
					   Xml::Services::sendGcode(fmt::format("M221 D{:d} S{:d}", *extruder, whole(percent)));
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
		m_watch = Xml::OmWatch({"move.speedFactor", "move.extruders", "tools"},
							   [this]
							   {
								   showSpeed();
								   showExtrusion();
							   });
	}

	void FactorsPresenter::showSpeed()
	{
		const float percent = 100 * Xml::MoveSource::speedFactor();
		m_speed.show(1, [](size_t) { return std::string(); }, [percent](size_t) { return percent; });
	}

	void FactorsPresenter::showExtrusion()
	{
		MODEL_LOCK();
		const auto numbers = extruders();
		const OM::Node& all = Xml::PrinterModel::root()["move"]["extruders"];
		m_extrusion.show(
			numbers.size(),
			[&numbers](size_t i) { return _("fine_tune.extruder", numbers[i]); },
			[&](size_t i) { return 100.0f * static_cast<float>(all[numbers[i]]["factor"].number()); });
	}

	std::vector<size_t> FactorsPresenter::extruders() const
	{
		std::vector<size_t> numbers;
		const OM::Node& all = Xml::PrinterModel::root()["move"]["extruders"];
		if (!m_tool)
		{
			for (size_t number = 0; number < all.size(); ++number)
			{
				if (all[number].exists())
				{
					numbers.push_back(number);
				}
			}
			return numbers;
		}
		const OM::Node& toolExtruders = Xml::PrinterModel::tool(*m_tool)["extruders"];
		for (size_t i = 0; i < toolExtruders.size(); ++i)
		{
			if (const int64_t number = toolExtruders[i].integer(-1);
				number >= 0 && all[static_cast<size_t>(number)].exists())
			{
				numbers.push_back(static_cast<size_t>(number));
			}
		}
		return numbers;
	}
} // namespace UI
