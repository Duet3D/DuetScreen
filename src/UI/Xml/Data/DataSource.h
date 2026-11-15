/*
 * DataSource.h
 *
 * A data source publishes names for XML that need C++. Printer data mostly doesn't: every object model field has an
 * `om.*` name (see Om/OmBindings.h). A data source is for what those can't express: logic over the object model (a
 * catalogue of every value a gauge can show, a summary of the current tool), and the UI's own state (the console,
 * navigation, customising). What it publishes, and the names, are declared in `fields()`; those names are the public
 * UI API (see UI_API_VERSION in Runtime/XmlUi.h) and renaming one breaks user bundles.
 *
 * XML writes a source's names `source.<name>.<field>`: `<name>Source.cpp`, `source.machine.printing` is in
 * MachineSource.cpp, so a name says where to look (a test keeps the rule).
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
 *         SensorSource() : DataSource("sensor") {}  // in SensorSource.cpp
 *         void fields(FieldVisitor& f) override { f.list("summary", m_rows, "…"); }  // source.sensor.summary
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
#include "UI/Xml/Binding/Names.h"
#include "UI/Xml/Om/OmWatch.h"
#include <memory>
#include <source_location>
#include <string>
#include <vector>

namespace UI::Xml
{
	/** Shared, so a reply that arrives after the source has stopped can tell (weak_from_this()) */
	class DataSource : public BasePresenter, public FieldProvider, public std::enable_shared_from_this<DataSource>
	{
	  public:
		/**
		 * @param name Its names are `source.<name>.<field>`; it is defined in `<Name>Source.cpp`
		 * @param where Where it is defined, recorded for the UI API reference
		 */
		explicit DataSource(std::string name, std::source_location where = std::source_location::current())
			: m_name(std::move(name))
			, m_file(where.file_name())
		{
		}

		/** Its name: its names are `source.<name>.<field>` */
		std::string_view getName() const override { return m_name; }

		/** What its names are written after: `source.<name>.` */
		std::string prefix() const { return std::string(Names::SOURCE) + m_name + "."; }

		/** The source file that defines it, e.g. "src/UI/Xml/Data/MachineSource.cpp" */
		const std::string& file() const { return m_file; }

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
		std::string m_name; // Its names are `source.<name>.<field>`
		std::string m_file; // The source file that defines it
		OmWatch m_watch;	// Calls update() as the object model changes, once watchObjectModel() has
	};
} // namespace UI::Xml
