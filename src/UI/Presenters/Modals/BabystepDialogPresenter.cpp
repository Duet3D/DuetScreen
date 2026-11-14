/*
 * BabystepDialogPresenter.cpp
 */

#include "BabystepDialogPresenter.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/View/XmlView.h"

namespace UI
{
	BabystepDialogPresenter* BabystepDialogPresenter::shared()
	{
		auto* view = findView<Xml::XmlView>("babystep_dialog");
		auto* presenter = view != nullptr ? dynamic_cast<BabystepDialogPresenter*>(view->getPresenter()) : nullptr;
		if (presenter == nullptr)
		{
			LOG_WARN("There is no babystepping dialog (a view called 'babystep_dialog' showing babystep_dialog.xml)");
		}
		return presenter;
	}

	BabystepDialogPresenter::BabystepDialogPresenter()
		: XmlPresenter("babystep_dialog")
	{
		action("babystep.close", "Close the babystepping dialog", [this](const Xml::ActionArgs&) { m_open.set(0); });
	}

	void BabystepDialogPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("open", m_open, "1 while the dialog is open");
	}

	void BabystepDialogPresenter::open()
	{
		m_open.set(1);
	}
} // namespace UI
