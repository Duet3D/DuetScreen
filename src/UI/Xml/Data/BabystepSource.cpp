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
#include <fmt/format.h>

namespace UI::Xml
{
	BabystepSource::BabystepSource()
		: DataSource("babystep")
		, m_step({.id = "babystep.step",
				  .storageKey = std::string(ID_MVS_BABYSTEPS),
				  .presets = {0.01f, 0.05f},
				  .min = 0.001f,
				  .max = 1.0f})
	{
		m_step.setUnit("mm");
	}

	void BabystepSource::fields(FieldVisitor& f)
	{
		f.field("usable", m_usable, "1 if Z can be babystepped: there is a Z axis, it is homed, and connected");
		f.field("reset_text", m_resetText, "The reset button's text, with the current offset, e.g. \"Reset 0.02 mm\"");
		f.field("dialog_open", m_dialogOpen, "1 while the shared babystepping dialog is open");
		m_step.fields(f);
	}

	void BabystepSource::onInit()
	{
		watchObjectModel({"move.axes"});
	}

	void BabystepSource::update()
	{
		m_step.setTitle(_("babystep.header"));
		MODEL_LOCK();
		const OM::Node& z = PrinterModel::axis('Z');
		m_usable.set(z.exists() && z["homed"].boolean() && OM::IsConnected() ? 1 : 0);
		m_resetText.set(_("babystep.reset", static_cast<float>(z["babystep"].number())));
	}

	void BabystepSource::clear()
	{
		m_usable.set(0);
		m_resetText.set(_("babystep.reset", 0.0f));
	}

	void BabystepSource::step(int direction) const
	{
		Services::sendGcode(fmt::format("M290 S{:g}", static_cast<float>(direction) * m_step.value()));
	}

	void BabystepSource::openDialog()
	{
		m_dialogOpen.set(1);
	}
} // namespace UI::Xml
