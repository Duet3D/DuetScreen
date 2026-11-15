/*
 * FactorsSource.cpp
 */

#include "FactorsSource.h"
#include "MoveSource.h"
#include "PrinterModel.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include <cmath>
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/** `percent` rounded to a whole number */
		uint32_t whole(float percent)
		{
			return static_cast<uint32_t>(std::lround(percent));
		}
	} // namespace

	FactorsSource::FactorsSource()
		: DataSource("factors")
		, m_speed({.id = "speed",
				   .title = _("fine_tune.speed_factor"),
				   .sliderMin = 1,
				   .sliderMax = 150,
				   .rangeFollowsValue = true,
				   .presets = {50, 75, 100, 125, 150},
				   .onSet =
					   [](size_t, float percent)
				   {
					   if (whole(percent) != whole(100 * MoveSource::speedFactor()))
					   {
						   Services::sendGcode(fmt::format("M220 S{:d}", whole(percent)));
					   }
				   }})
		, m_extrusion({.id = "extrusion",
					   .title = _("fine_tune.extruder_header"),
					   .sliderMin = 1,
					   .sliderMax = 200,
					   .presets = {90, 95, 100, 105, 110},
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
									   PrinterModel::root()["move"]["extruders"][numbers[index]]["factor"].number());
							   }
						   }
						   if (extruder && whole(percent) != whole(100 * factor))
						   {
							   Services::sendGcode(fmt::format("M221 D{:d} S{:d}", *extruder, whole(percent)));
						   }
					   }})
	{
	}

	void FactorsSource::fields(FieldVisitor& f)
	{
		m_speed.fields(f);
		m_extrusion.fields(f);
		f.field("extrusion_all",
				m_all,
				"1 while the extrusion dialog shows every extruder, 0 while only the tool's it was opened for "
				"(actions.factors.extrusion_all)");
		f.field("extrusion_all_shown",
				m_allShown,
				"1 if the extrusion dialog can show every extruder or only the tool's: it was opened for a tool that "
				"doesn't use them all");
	}

	void FactorsSource::showAllExtruders(bool all)
	{
		m_all.set(all ? 1 : 0);
		showExtrusion();
	}

	void FactorsSource::onInit()
	{
		watchObjectModel({"move.speedFactor", "move.extruders", "tools", "state.machineMode"});
	}

	void FactorsSource::update()
	{
		showSpeed();
		showExtrusion();
	}

	void FactorsSource::clear()
	{
		m_speed.close();
		m_extrusion.close();
		m_speed.show(0, {}, {});
		m_extrusion.show(0, {}, {});
	}

	void FactorsSource::openSpeed()
	{
		showSpeed();
		m_speed.open();
	}

	void FactorsSource::openExtrusion(std::optional<size_t> tool)
	{
		m_tool = tool;
		m_all.set(0);
		showExtrusion();
		m_extrusion.open();
	}

	void FactorsSource::showSpeed()
	{
		const float percent = 100 * MoveSource::speedFactor();
		bool cnc = false;
		{
			MODEL_LOCK();
			cnc = PrinterModel::root()["state"]["machineMode"].text() == "CNC";
		}
		// A CNC machine calls it the feed override
		m_speed.setTitle(cnc ? _("move.feed_override") : _("move.speed_factor"));
		m_speed.show(
			1,
			[](size_t) { return std::string(); },
			[percent](size_t) { return percent; },
			[](size_t) { return _("move.override_hint"); });
	}

	void FactorsSource::showExtrusion()
	{
		MODEL_LOCK();
		const auto numbers = extruders();
		const OM::Node& all = PrinterModel::root()["move"]["extruders"];
		m_allShown.set(m_tool && extruders(true).size() > extruders(false).size() ? 1 : 0);
		m_extrusion.show(
			numbers.size(),
			[&numbers](size_t i) { return _("fine_tune.extruder", numbers[i]); },
			[&](size_t i) { return 100.0f * static_cast<float>(all[numbers[i]]["factor"].number()); },
			[&numbers](size_t i) { return PrinterModel::toolsUsing("extruders", numbers[i]); });
	}

	std::vector<size_t> FactorsSource::extruders() const
	{
		return extruders(m_all.get() != 0);
	}

	std::vector<size_t> FactorsSource::extruders(bool every) const
	{
		std::vector<size_t> numbers;
		const OM::Node& all = PrinterModel::root()["move"]["extruders"];
		if (!m_tool || every)
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
		const OM::Node& toolExtruders = PrinterModel::tool(*m_tool)["extruders"];
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
} // namespace UI::Xml
