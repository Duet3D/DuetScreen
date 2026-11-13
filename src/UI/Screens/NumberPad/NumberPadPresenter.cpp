/*
 * NumberPadPresenter.cpp
 */

#include "NumberPadPresenter.h"

namespace UI
{
	NumberPadPresenter::NumberPadPresenter()
		: XmlPresenter("numberpad")
		, m_pad("pad")
	{
		m_pad.setOnConfirmed([this]() { close(); });
		m_pad.setOnDropped([this]() { close(); });

		action("numberpad.close",
			   "Close the dialog without answering, e.g. when it is dismissed",
			   [this](const Xml::ActionArgs&) { close(); });
		for (auto& [name, description, handler] : Xml::NumberEntry::actions({&m_pad}))
		{
			action(name, description, handler);
		}
	}

	void NumberPadPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("open", m_open, "1 while the dialog is open");
		m_pad.fields(f);
	}

	void NumberPadPresenter::ask(Xml::NumberRequest request)
	{
		m_pad.ask(std::move(request));
		m_open.set(1);
	}

	void NumberPadPresenter::ask(Xml::TextRequest request)
	{
		m_pad.ask(std::move(request));
		m_open.set(1);
	}

	void NumberPadPresenter::close()
	{
		m_open.set(0);
		m_pad.drop();
	}
} // namespace UI
