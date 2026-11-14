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
	}

	void FactorsSource::onInit()
	{
		watchObjectModel({"move.speedFactor", "move.extruders", "tools"});
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
		showExtrusion();
		m_extrusion.open();
	}

	FactorDialog* FactorsSource::dialog(std::string_view id)
	{
		for (FactorDialog* dialog : {&m_speed, &m_extrusion})
		{
			if (dialog->id() == id)
			{
				return dialog;
			}
		}
		return nullptr;
	}

	void FactorsSource::showSpeed()
	{
		const float percent = 100 * MoveSource::speedFactor();
		m_speed.show(1, [](size_t) { return std::string(); }, [percent](size_t) { return percent; });
	}

	void FactorsSource::showExtrusion()
	{
		MODEL_LOCK();
		const auto numbers = extruders();
		const OM::Node& all = PrinterModel::root()["move"]["extruders"];
		m_extrusion.show(
			numbers.size(),
			[&numbers](size_t i) { return _("fine_tune.extruder", numbers[i]); },
			[&](size_t i) { return 100.0f * static_cast<float>(all[numbers[i]]["factor"].number()); });
	}

	std::vector<size_t> FactorsSource::extruders() const
	{
		std::vector<size_t> numbers;
		const OM::Node& all = PrinterModel::root()["move"]["extruders"];
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
