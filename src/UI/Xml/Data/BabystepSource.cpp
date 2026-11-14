/*
 * BabystepSource.cpp
 */

#include "BabystepSource.h"
#include "ObjectModel/PrinterStatus.h"
#include "PrinterModel.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/** The babystep sizes, in millimetres or in inches */
		ValueSelector::Presets steps(bool metric)
		{
			const std::string key(ID_MVS_BABYSTEPS);
			return metric
					   ? ValueSelector::Presets{.storageKey = key, .values = {0.01f, 0.05f}, .min = 0.001f, .max = 1.0f}
					   : ValueSelector::Presets{
							 .storageKey = key + "_in", .values = {0.0005f, 0.002f}, .min = 0.0001f, .max = 0.04f};
		}
	} // namespace

	BabystepSource::BabystepSource()
		: DataSource("babystep")
		, m_step({.id = "babystep.step", .presets = steps(true)})
	{
	}

	void BabystepSource::fields(FieldVisitor& f)
	{
		f.field("usable", m_usable, "1 if Z can be babystepped: there is a Z axis, it is homed, and connected");
		f.field("dialog_open", m_dialogOpen, "1 while the shared babystepping dialog is open");
		m_step.fields(f);
	}

	void BabystepSource::onInit()
	{
		watchObjectModel({"move.axes"});
	}

	void BabystepSource::update()
	{
		m_step.use(steps(Units::getCurrentDisplayedUnitSystem() == Units::UnitSystem::Metric));
		m_step.setTitle(_("babystep.header"));
		m_step.setUnit(Units::getDisplayedDistanceUnit());
		MODEL_LOCK();
		const OM::Node& z = PrinterModel::axis('Z');
		m_usable.set(z.exists() && z["homed"].boolean() && OM::IsConnected() ? 1 : 0);
	}

	void BabystepSource::clear()
	{
		m_usable.set(0);
	}

	void BabystepSource::step(int direction) const
	{
		// The step is in the displayed unit; M290 takes the Duet's (G20/G21), as moves do
		Services::sendGcode(fmt::format(
			"M290 S{:g}", Units::convertDisplayedDistanceToDuetUnits(static_cast<float>(direction) * m_step.value())));
	}

	void BabystepSource::openDialog()
	{
		m_dialogOpen.set(1);
	}
} // namespace UI::Xml
