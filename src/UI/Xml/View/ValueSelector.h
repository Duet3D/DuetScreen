/*
 * ValueSelector.h
 *
 * The fields and actions behind value_selector.xml: a value with - and + beside it, tapped to type one in, and a row
 * of preset values below, tapped to use one and held to change it. A presenter owns one ValueSelector per value,
 * declares its fields, and handles its actions:
 *
 *     MyPresenter::MyPresenter() : XmlPresenter("my")
 *     {
 *         for (auto& [name, description, handler] : Xml::ValueSelector::actions({&m_distance}))
 *             action(name, description, handler);
 *     }
 *     void MyPresenter::fields(Xml::FieldVisitor& f) { m_distance.fields(f); }
 *
 *     <value_selector selector="distance" title="distance.title" value="distance.value" choices="distance.choices"/>
 *
 * Its fields are named under the selector's id (e.g. "distance.title"). They are declared in fields(), and its
 * actions in actions(); both are listed in the UI API reference (docs/XML_UI_API.md).
 *
 * The presets and the value are kept in storage under the selector's storage key, so they survive a restart.
 */

#pragma once

#include "Actions.h"
#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <initializer_list>
#include <limits>
#include <string>
#include <utility>
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

		StringSubject<16> text;			  // The preset, e.g. "10"
		ValueSelector* owner = nullptr;	  // The selector it belongs to; not shown
	};

	class ValueSelector : public FieldProvider
	{
	  public:
		/** How a selector is set up */
		struct Settings
		{
			std::string id;								   // Prefix of its fields, and its actions' arg
			std::string storageKey;						   // Where its presets and value are kept; empty for nowhere
			std::vector<float> presets;					   // The presets until others are stored
			float min = 0.0f;							   // Lowest value
			float max = std::numeric_limits<float>::max(); // Highest value
			float step = 1.0f;							   // What - and + take off or add
		};

		/** Create the selector, with the presets and value last stored, else the defaults and `min` */
		explicit ValueSelector(Settings settings);

		/** Declare `<id>.title`, `<id>.value` and `<id>.choices` */
		void fields(FieldVisitor& f) override;

		/** Show `title` above the value */
		void setTitle(std::string_view title);

		/** The value chosen */
		float value() const { return m_value; }

		/** Use `value`, kept within the limits, and store it */
		void setValue(float value);

		/** One selector action, to register with XmlPresenter::action() */
		struct Action
		{
			std::string name;		 // Action name, e.g. "selector.edit"
			std::string description; // What it does, for the UI API reference
			ActionHandler handler;	 // Runs it
		};

		/** The selector actions (see the file comment) of any of `selectors`, to register */
		static std::vector<Action> actions(std::initializer_list<ValueSelector*> selectors);

	  private:
		/** Change preset `index` to `value`, and store the presets */
		void setPreset(size_t index, float value);

		/** Show the presets */
		void showPresets();

		/** Ask for a value with `header`, then call `onConfirm` with it */
		void ask(std::string header, float start, std::function<void(float)> onConfirm);

		Settings m_settings;		   // How it is set up
		std::vector<float> m_presets;  // The presets
		float m_value = 0.0f;		   // The value chosen

		StringSubject<64> m_title;		  // `<id>.title`
		StringSubject<16> m_valueText;	  // `<id>.value`
		List<ValueChoiceRow> m_choices;	  // `<id>.choices`
	};
} // namespace UI::Xml
