/*
 * DataSource.h
 *
 * A data source publishes one area of the object model (fans, tools, …) as global subjects and lists, so any XML
 * view can show printer data without C++. What it publishes, and the names, are declared in `fields()`; those names
 * are the public UI API (see UI_API_VERSION in Runtime/XmlUi.h) and renaming one breaks user bundles.
 *
 * A data source is a presenter for the whole UI rather than for one view: it is created when the XML UI starts,
 * stays active, and listens to Model events (registered in `onInit()`) for as long as it lives. Connecting re-reads
 * everything (`update()`); disconnecting empties everything (`clear()`).
 *
 * Writing one:
 *
 *     class FanSource : public DataSource
 *     {
 *       public:
 *         FanSource() : DataSource("fans") {}
 *         void fields(FieldVisitor& f) override { f.list("fans", m_fans, "One row per fan"); }
 *         void update() override;  // m_fans.sync(...)
 *         void clear() override { m_fans.clear(); }
 *       protected:
 *         void onInit() override { registerEventListener<EventType::FanData>(this, &FanSource::update); }
 *       private:
 *         List<FanRow> m_fans;
 *     };
 *
 * and add it to the list in DataSources.cpp.
 */

#pragma once

#include "UI/Core/Presenter.h"
#include "UI/Xml/Binding/Fields.h"
#include <string>

namespace UI::Xml
{
	class DataSource : public BasePresenter, public FieldProvider
	{
	  public:
		/** @param name Name used in logs */
		explicit DataSource(std::string name)
			: m_name(std::move(name))
		{
		}

		/** Name used in logs */
		std::string_view getName() const override { return m_name; }

		/** Re-read everything this source publishes from the object model */
		virtual void update() = 0;

		/** Publish nothing: empty every list and reset every subject (e.g. when disconnected) */
		virtual void clear() = 0;

	  protected:
		/** Called when the printer connects: re-read everything */
		void onConnect() override { update(); }

		/** Called when the printer disconnects: forget everything */
		void onDisconnect() override { clear(); }

	  private:
		std::string m_name; // Name used in logs
	};
} // namespace UI::Xml
