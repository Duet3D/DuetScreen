/*
 * FactorDialog.h
 *
 * The fields and actions behind factors_dialog.xml: a dialog of percentages, such as the speed factor or each
 * extruder's extrusion factor, each on a slider whose value can be tapped to type it (the number pad is the dialog's
 * next step). A presenter
 * owns one FactorDialog per dialog, fills in its factors, declares its fields and handles its actions, as with a
 * ValueSelector:
 *
 *     MyPresenter::MyPresenter() : XmlPresenter("my"), m_speed({.id = "speed", ...})
 *     {
 *         for (auto& [name, description, handler] : FactorDialog::actions({&m_speed}))
 *             action(name, description, handler);
 *     }
 *     void MyPresenter::fields(Xml::FieldVisitor& f) { m_speed.fields(f); }
 *
 *     <factors_dialog dialog="speed" open="speed.open" title="speed.title" factors="speed.factors" .../>
 *
 * Its fields are named under the dialog's id (e.g. "speed.factors"). They are declared in fields(), and its actions in
 * actions(); both are listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/Actions.h"
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI
{
	class FactorDialog;

	/** One percentage of a FactorDialog */
	class FactorRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<32> label; // What it is, e.g. "Extruder 0"
		Xml::IntSubject value;		  // The percentage, e.g. 100
		Xml::IntSubject min;		  // The bottom of its slider
		Xml::IntSubject max;		  // The top of its slider
		FactorDialog* owner = nullptr; // The dialog it belongs to; not shown
	};

	class FactorDialog : public Xml::FieldProvider
	{
	  public:
		/** How a dialog is set up */
		struct Settings
		{
			std::string id;							  // Prefix of its fields, and its actions' arg
			std::string title;						  // The dialog's title, e.g. "Speed Factor"
			int32_t sliderMin = 0;					  // The bottom of each slider, and the least that can be typed
			int32_t sliderMax = 200;				  // The top of each slider; more can be typed
			bool rangeFollowsValue = false;			  // Slide 50 either side of the value (from sliderMin)
			std::function<void(size_t, float)> onSet; // Use percentage `value` for factor `index`
		};

		/** Create the dialog, closed and with no factors */
		explicit FactorDialog(Settings settings);

		/** Declare `<id>.open`, `<id>.title` and `<id>.factors` */
		void fields(Xml::FieldVisitor& f) override;

		/** Show `count` factors, called `labelOf(i)` with percentage `valueOf(i)` */
		void show(size_t count,
				  const std::function<std::string(size_t)>& labelOf,
				  const std::function<float(size_t)>& valueOf);

		/** Open the dialog */
		void open();

		/** Close the dialog */
		void close();

		/** One dialog action, to register with XmlPresenter::action() */
		struct Action
		{
			std::string name;		  // Action name, e.g. "factor.set"
			std::string description;  // What it does, for the UI API reference
			Xml::ActionHandler handler; // Runs it
		};

		/**
		 * The dialog actions (see the file comment) of any of `dialogs`, to register. A presenter registers them once,
		 * for all its dialogs.
		 */
		static std::vector<Action> actions(std::initializer_list<FactorDialog*> dialogs);

	  private:
		/** Show `value` for factor `row`, moving its slider's range if it follows the value */
		void setRow(FactorRow& row, int32_t value) const;

		/** Use percentage `value` for factor `index`, as the user chose it */
		void set(size_t index, float value);

		Settings m_settings;			// How it is set up
		Xml::IntSubject m_open;			// `<id>.open`
		Xml::StringSubject<64> m_title; // `<id>.title`
		Xml::List<FactorRow> m_factors; // `<id>.factors`
	};
} // namespace UI
