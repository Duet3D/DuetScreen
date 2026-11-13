/*
 * Services.cpp
 */

#include "Services.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Modals/NumberPadPresenter.h"
#include "UI/Xml/View/XmlView.h"
#include <string>

namespace UI::Xml::Services
{
	static std::function<void(std::string_view)> s_gcodeSender; // Replacement sender, or empty for the Duet

	void sendGcode(std::string_view gcode)
	{
		// Lines are newline-terminated on the wire; XML authors shouldn't have to write "\n"
		std::string line(gcode);
		if (line.empty() || line.back() != '\n')
		{
			line += '\n';
		}

		LOG_DBG("XML UI sending '{:s}'", gcode);
		if (s_gcodeSender)
		{
			s_gcodeSender(line);
			return;
		}
		Comm::DUET.SendGcode(line);
	}

	namespace
	{
		/** The shared number pad's presenter, or nullptr if there is none */
		NumberPadPresenter* sharedNumberPad()
		{
			auto* view = findView<XmlView>("numberpad");
			auto* presenter = view != nullptr ? dynamic_cast<NumberPadPresenter*>(view->getPresenter()) : nullptr;
			if (presenter == nullptr)
			{
				LOG_WARN("There is no shared number pad (a view called 'numberpad' showing numberpad.xml)");
			}
			return presenter;
		}
	} // namespace

	bool askNumber(NumberRequest request)
	{
		auto* pad = sharedNumberPad();
		if (pad == nullptr)
		{
			return false;
		}
		pad->ask(std::move(request));
		return true;
	}

	bool askText(TextRequest request)
	{
		auto* pad = sharedNumberPad();
		if (pad == nullptr)
		{
			return false;
		}
		pad->ask(std::move(request));
		return true;
	}

	void setGcodeSender(std::function<void(std::string_view)> sender)
	{
		s_gcodeSender = std::move(sender);
	}

} // namespace UI::Xml::Services
