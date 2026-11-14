/*
 * DataSource.h
 *
 * A data source publishes names for XML that need C++. Printer data mostly doesn't: every object model field has an
 * `om.*` name (see Om/OmBindings.h). A data source is for what those can't express: logic over the object model (a
 * catalogue of every value a gauge can show, a summary of the current tool), and the UI's own state (the console,
 * navigation, customising). What it publishes, and the names, are declared in `fields()`; those names are the public
 * UI API (see UI_API_VERSION in Runtime/XmlUi.h) and renaming one breaks user bundles.
 *
 * A data source is a presenter for the whole UI rather than for one view: it is created when the XML UI starts, stays
 * active, and listens to Model events (registered in `onInit()`) for as long as it lives. Connecting re-reads
 * everything (`update()`); disconnecting empties everything (`clear()`).
 *
 * Writing one that reads the object model:
 *
 *     class SensorSource : public DataSource
 *     {
 *       public:
 *         SensorSource() : DataSource("sensors") {}
 *         void fields(FieldVisitor& f) override { f.list("sensor_summary", m_rows, "…"); }
 *         void update() override;  // MODEL_LOCK(), read OM::Tree::get().root()["sensors"], m_rows.sync(...)
 *         void clear() override { m_rows.clear(); }
 *       protected:
 *         void onInit() override { watchObjectModel({"sensors"}); }
 *       private:
 *         List<SensorRow> m_rows;
 *     };
 *
 * and add it to the list in DataSources.cpp.
 */

#pragma once

#include "UI/Core/Presenter.h"
#include "UI/Xml/Binding/Fields.h"
#include <string>
#include <vector>

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

		/** Stops watching the object model */
		~DataSource() override;

		/** Name used in logs */
		std::string_view getName() const override { return m_name; }

		/** Re-read everything this source publishes */
		virtual void update() = 0;

		/** Publish nothing: empty every list and reset every subject (e.g. when disconnected) */
		virtual void clear() = 0;

	  protected:
		/** Called when the printer connects: re-read everything */
		void onConnect() override { update(); }

		/** Called when the printer disconnects: forget everything */
		void onDisconnect() override { clear(); }

		/**
		 * Call update() whenever the object model changes at, below or above one of `paths`, e.g. "fans" or
		 * "heat.heaters", once its changes have reached the `om.*` names. Call from onInit().
		 */
		void watchObjectModel(std::vector<std::string> paths);

	  private:
		std::string m_name;	 // Name used in logs
		size_t m_watch = 0; // The object model watcher, or 0 if none
	};
} // namespace UI::Xml
