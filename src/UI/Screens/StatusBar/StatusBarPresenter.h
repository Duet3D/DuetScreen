/*
 * StatusBarPresenter.h
 *
 * Behaviour for the status bar (assets/ui/components/status_bar.xml): the connection and printer state, the
 * printer's name, and the current tool's temperatures.
 *
 * Fields
 *   status            connection and state, e.g. "USB - Idle"
 *   name              the printer's name
 *   has_tool          1 if there is a current tool with heaters, else 0
 *   tool              the current tool, e.g. "T0:", or empty if it has no heaters
 *   heaters           the current tool's heaters' current and target temperatures, e.g. " 210°C/215°C 60°C/60°C"
 *   fault             1 if any of the current tool's heaters has a fault, else 0
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace OM
{
	enum class PrinterStatus;
}

namespace UI
{
	class StatusBarPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		StatusBarPresenter();

		/** Declare the status bar's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for name, state and heater changes */
		void onInit() override;

		/** Show the current state */
		void onActivate() override;

		/** Show "connected" until the printer reports its state */
		void onConnect() override;

		/** Show "disconnected" and forget the printer */
		void onDisconnect() override;

	  private:
		/** Show the printer's name */
		void updateName();

		/** Show the printer's state */
		void updateStatus(const OM::PrinterStatus& status);

		/** Show the current tool's temperatures */
		void updateHeaters();

		/** Show "<connection> - <state>" with the translation of `state` */
		void showStatus(std::string_view state);

		Xml::StringSubject<64> m_status;		  // Connection and state
		Xml::StringSubject<64> m_name;			  // Printer name
		Xml::IntSubject m_hasTool;				  // 1 if there is a current tool with heaters
		Xml::StringSubject<16> m_tool;			  // "T0:", or empty
		Xml::StringSubject<160> m_heaters;		  // Current tool's heaters' temperatures
		Xml::IntSubject m_fault;				  // 1 if any of them has a fault
	};
} // namespace UI
