/*
 * MachineSource.h
 *
 * Publishes the printer's state, which mixes the connection (connecting, disconnected) with the state the printer
 * reports, and how the screen is connected to it. Its name is the object model's, om.network.name.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"
#include <string_view>

namespace OM
{
	enum class PrinterStatus;
}

namespace UI::Xml
{
	class MachineSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		MachineSource();

		/** Declare `source.machine.*` */
		void fields(FieldVisitor& f) override;

		/** Read the printer's state */
		void update() override;

		/** The printer has gone: no name, and the state is "disconnected" */
		void clear() override;

	  protected:
		/** Listen for name and state changes */
		void onInit() override;

		/** The printer is connected: read everything, and say so until its state is known */
		void onConnect() override;

	  private:
		/** The printer's state changed */
		void statusChanged(const OM::PrinterStatus& status);

		/** Show the state key `state`, e.g. "idle", translated, with the connection */
		void showState(std::string_view state);

		IntSubject m_connected;			 // 1 while connected
		StringSubject<16> m_status;		 // State key
		IntSubject m_printing;			 // 1 while a job is running and not paused
		IntSubject m_jobActive;			 // 1 while there is a job, even paused
		StringSubject<32> m_connection;	 // How the screen is connected, translated
		StringSubject<32> m_state;		 // The state, translated
		StringSubject<64> m_summary;	 // Connection and state, e.g. "USB - Idle"
	};
} // namespace UI::Xml
