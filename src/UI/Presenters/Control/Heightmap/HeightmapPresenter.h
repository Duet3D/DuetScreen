/*
 * HeightmapPresenter.h
 *
 * Behaviour for the Bed Levelling tab (assets/ui/components/control/heightmap/heightmap.xml): the height map shown, the
 * height map files on the printer, and their statistics.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md). Levelling and probing use the global `gcode` action (G32, G29).
 */

#pragma once

#include "ObjectModel/Files.h"
#include "ObjectModel/Heightmap.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Binding/Surface.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <memory>
#include <mutex>

namespace UI
{
	/** One height map file */
	class HeightmapFileRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<64> name;	  // File name
		Xml::IntSubject loaded;			  // 1 if the printer is using it
		Xml::StringSubject<16> loadText;  // "Load" or "Unload"
	};

	class HeightmapPresenter : public Xml::XmlPresenter
	{
	  public:
		/** How values are coloured */
		enum class RenderMode
		{
			Fixed = 0, // ±0.25 mm
			Auto,	   // The map's own range
		};

		/** Create the presenter */
		HeightmapPresenter();

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for height maps, files, axes and the printer's state */
		void onInit() override;

		/** Ask for the height maps and show the one in use */
		void onActivate() override;

		/** Ask for the height maps again */
		void onConnect() override;

		/** Forget every height map */
		void onDisconnect() override;

	  private:
		/** The X or Y span of a height map's axis, allowing for the current tool's offset */
		struct AxisRange
		{
			float min = 0.0f; // Lowest
			float max = 0.0f; // Highest

			/** Whether two spans are the same */
			bool operator==(const AxisRange& other) const = default;
		};

		/** Show `map`, or nothing if it is null; samples it on the calling thread, so may be called off the UI thread */
		void show(const OM::HeightmapPtr& map);

		/** Show nothing, with empty statistics */
		void clear();

		/** Load `map` from the printer and show it when it arrives */
		void load(const OM::HeightmapPtr& map);

		/** Ask for the list of height map files */
		void requestFiles();

		/** Show the height map files, marking the one in use */
		void showFiles();

		/** Show whether the bed can be levelled */
		void showUsable();

		/** The axes changed: show the map again if its span moved */
		void axesChanged();

		/** The file a maps row is about, or nullptr */
		OM::FileSystem::ItemPtr fileOf(const Xml::ActionArgs& args) const;

		std::mutex m_mapMutex;						// Guards m_map, which is loaded on another thread
		OM::HeightmapPtr m_map;						// The map shown, or nullptr
		OM::FileSystem::ItemList m_files;			// Height map files, by name
		RenderMode m_mode = RenderMode::Fixed;		// How values are coloured
		AxisRange m_xRange;							// X span the map was shown with
		AxisRange m_yRange;							// Y span the map was shown with

		Xml::Surface m_surface;						// The map shown
		Xml::StringSubject<96> m_title;				// "Heightmap: …"
		Xml::List<HeightmapFileRow> m_maps;			// The files
		Xml::StringSubject<48> m_points;			// Statistics, as text
		Xml::StringSubject<48> m_area;				// …
		Xml::StringSubject<48> m_min;				// …
		Xml::StringSubject<48> m_max;				// …
		Xml::StringSubject<48> m_mean;				// …
		Xml::StringSubject<48> m_rms;				// …
		Xml::IntSubject m_modeSubject;				// `mode`
		Xml::IntSubject m_levellingUsable;			// 1 if the bed can be levelled
	};
} // namespace UI
