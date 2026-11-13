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

		/** A number pad in `obj`'s widget tree, or nullptr */
		ModalNumberPad* numberPadIn(lv_obj_t* obj)
		{
			if (lv_obj_get_user_data(obj) != nullptr)
			{
				if (auto* pad = dynamic_cast<ModalNumberPad*>(LvObj::fromPtr(obj)))
				{
					return pad;
				}
			}
			for (uint32_t i = 0; i < lv_obj_get_child_count(obj); i++)
			{
				if (auto* pad = numberPadIn(lv_obj_get_child(obj, static_cast<int32_t>(i))))
				{
					return pad;
				}
			}
			return nullptr;
		}

		/** The number pad nearest `asker`: each enclosing widget in turn, then the shared one; or nullptr */
		ModalNumberPad* nearestNumberPad(lv_obj_t* asker)
		{
			for (lv_obj_t* scope = asker; scope != nullptr; scope = lv_obj_get_parent(scope))
			{
				if (auto* pad = numberPadIn(scope))
				{
					return pad;
				}
			}
			return findView<ModalNumberPad>("numberpad");
		}
	} // namespace

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
		auto* pad = nearestNumberPad(asker);
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
		auto* pad = nearestNumberPad(asker);
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
