/*
 * ValueSelector.h
 *
 * A value chosen from a row of presets, which the user can change: the jog distance and speed, the extruder's feed
 * distance and rate, the babystep size. A data source owns one ValueSelector per value and declares its fields, so
 * every view that shows the value shares it:
 *
 *     JogSource::JogSource() : DataSource("jog"), m_distance({.id = "jog.distance", ...}) {}
 *     void JogSource::fields(FieldVisitor& f) { m_distance.fields(f); }  // source.jog.distance.*
 *
 *     <value_selector selector="jog.distance" title="source.jog.distance.title"
 *                     value="source.jog.distance.value" choices="source.jog.distance.choices"/>
 *
 * A selector's id is `<source>.<name>`: its fields are `source.<source>.<name>.title`, `.value` and `.choices`, and the
 * id is the `arg` of the global actions that change it, `actions.selector.*` (SelectorActions.h). Actions raised from a
 * row of its `choices` find it through the row.
 *
 * The presets are kept in storage under their set's storage key, so they survive a restart; so is the value, if
 * the selector keeps it. A selector whose values have a unit has a set of presets for each unit, and its source picks
 * the set in use with use(): the jog distances in millimetres or in inches, say, as the screen shows lengths. Each
 * set keeps its own presets, so changing units never turns 50 mm into 50 inches.
 */

#pragma once

#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	class ValueSelector;

	/** One preset of a ValueSelector */
	class ValueChoiceRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<16> text;			// The preset, e.g. "10"
		IntSubject selected;			// 1 if it is the value chosen
		ValueSelector* owner = nullptr; // The selector it belongs to; not shown
	};

	class ValueSelector : public FieldProvider
	{
	  public:
		/** A set of presets, in one unit, and the limits its values keep to, e.g. the jog distances in millimetres */
		struct Presets
		{
			std::string storageKey;						   // Where the presets (and value) are kept
			std::vector<float> values;					   // The presets until others are stored
			size_t initial = 0;							   // The preset chosen at first
			float min = 0.0f;							   // Lowest value
			float max = std::numeric_limits<float>::max(); // Highest value
			float step = 1.0f;							   // What - and + take off or add
		};

		/** How a selector is set up */
		struct Settings
		{
			std::string id;			// `<source>.<name>`, e.g. "jog.distance"
			Presets presets;		// The presets used until use() picks others
			bool keepValue = false; // Store the value chosen, so it survives a restart
		};

		/** Create the selector, with the presets (and value) last stored, else the defaults */
		explicit ValueSelector(Settings settings);

		/** Forget the selector */
		~ValueSelector() override;

		ValueSelector(const ValueSelector&) = delete;
		ValueSelector& operator=(const ValueSelector&) = delete;

		/** The live selector with id `id`, or nullptr */
		static ValueSelector* find(std::string_view id);

		/** Its id, `<source>.<name>` */
		const std::string& id() const { return m_settings.id; }

		/** Declare `<name>.title`, `<name>.value` and `<name>.choices`, for its source to publish */
		void fields(FieldVisitor& f) override;

		/**
		 * Use another set of presets, e.g. those in inches once the screen shows inches: those last stored under its
		 * storage key, else its defaults, and its value. Nothing changes if the set is already in use.
		 */
		void use(const Presets& presets);

		/** Show `title` above the value */
		void setTitle(std::string_view title);

		/** Show `unit` beside the value on the number pad, e.g. "mm" */
		void setUnit(std::string_view unit);

		/** The value chosen */
		float value() const { return m_value; }

		/** Use `value`, kept within the limits */
		void setValue(float value);

		/** Use preset `index` */
		void pick(size_t index);

		/** Step the value up (`direction` 1) or down (-1) */
		void step(int direction);

		/** Ask for a value with the number pad, and use it */
		void edit();

		/** Ask for a new value for preset `index` with the number pad, and use it */
		void editPreset(size_t index);

	  private:
		/** Show the presets in use (m_settings.presets) as last stored, else their defaults, and their value */
		void load();

		/** Change preset `index` to `value`, and store the presets */
		void setPreset(size_t index, float value);

		/** Show the presets, and which is chosen */
		void showPresets();

		Settings m_settings;		  // How it is set up
		std::vector<float> m_presets; // The presets
		float m_value = 0.0f;		  // The value chosen
		std::string m_unit;			  // Shown on the number pad

		StringSubject<64> m_title;		// `<name>.title`
		StringSubject<16> m_valueText;	// `<name>.value`
		StringSubject<16> m_unitText;	// `<name>.unit`
		List<ValueChoiceRow> m_choices; // `<name>.choices`
	};
} // namespace UI::Xml
