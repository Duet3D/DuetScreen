/*
 * Services.cpp
 */

#include "Services.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Core/Navigation.h"
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
		/** Show `request` on `pad` */
		void ask(ModalNumberPad& pad, NumberRequest request)
		{
			pad.setHeader(request.header);
			pad.setMinValue(request.min);
			pad.setMaxValue(request.max);
			pad.setConfirmCallback(std::move(request.onConfirm));
			if (request.value)
			{
				pad.setValue(*request.value);
			}
			else
			{
				pad.clear();
			}
			openModal(&pad);
		}

	} // namespace

	ModalNumberPad* numberPadFor(lv_obj_t* asker)
	{
		// A pad serves what is inside the widget it was placed in, so a page's own pad isn't found from another page
		const std::vector<ModalNumberPad*> pads = findViews<ModalNumberPad>();
		for (lv_obj_t* scope = asker; scope != nullptr; scope = lv_obj_get_parent(scope))
		{
			for (ModalNumberPad* pad : pads)
			{
				if (lv_obj_get_parent(pad->getBackdropPtr()) == scope)
				{
					return pad;
				}
			}
		}
		return findView<ModalNumberPad>("numberpad");
	}

	bool askNumber(std::string_view numberPad, NumberRequest request)
	{
		auto* pad = findView<ModalNumberPad>(numberPad);
		if (pad == nullptr)
		{
			LOG_WARN("No number pad called '{:s}'", numberPad);
			return false;
		}
		ask(*pad, std::move(request));
		return true;
	}

	bool askNumber(lv_obj_t* asker, NumberRequest request)
	{
		auto* pad = numberPadFor(asker);
		if (pad == nullptr)
		{
			LOG_WARN("No number pad to ask for {:s}", request.header);
			return false;
		}
		ask(*pad, std::move(request));
		return true;
	}

	bool askText(lv_obj_t* asker, TextRequest request)
	{
		auto* pad = numberPadFor(asker);
		if (pad == nullptr)
		{
			LOG_WARN("No number pad to ask for {:s}", request.header);
			return false;
		}
		pad->setHeader(request.header);
		pad->setConfirmCallback(std::move(request.onConfirm));
		pad->setText(request.text);
		openModal(pad);
		return true;
	}

	void setGcodeSender(std::function<void(std::string_view)> sender)
	{
		s_gcodeSender = std::move(sender);
	}

} // namespace UI::Xml::Services
