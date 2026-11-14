/*
 * FactorDialog.h
 *
 * The fields behind factors_dialog.xml: a dialog of percentages, such as the speed factor or each extruder's
 * extrusion factor, each on a slider whose value can be tapped to type it (the number pad is the dialog's next step).
 * Its owner (FactorsSource) fills in its factors and declares its fields, named under the dialog's id:
 *
 *     void FactorsSource::fields(FieldVisitor& f) { m_speed.fields(f); }  // source.factors.speed.open, ...
 *
 *     <factors_dialog open="source.factors.speed.open" title="source.factors.speed.title"
 *                     factors="source.factors.speed.factors"/>
 *
 * The global action `actions.factors.set`, raised from a row, which knows its dialog, sets a factor
 * (src/UI/Presenters/Modals/FactorsActions.h). The dialog closes itself when the user closes it (DsDialog.h).
 */

#pragma once

#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <functional>
#include <string>

namespace UI::Xml
{
	class FactorDialog;

	/** One percentage of a FactorDialog */
	class FactorRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<32> label;	   // What it is, e.g. "Extruder 0"
		IntSubject value;			   // The percentage, e.g. 100
		IntSubject min;				   // The bottom of its slider
		IntSubject max;				   // The top of its slider
		FactorDialog* owner = nullptr; // The dialog it belongs to; not shown
	};

	class FactorDialog : public FieldProvider
	{
	  public:
		/** How a dialog is set up */
		struct Settings
		{
			std::string id;							  // Prefix of its fields, e.g. "speed"
			std::string title;						  // The dialog's title, e.g. "Speed Factor"
			int32_t sliderMin = 0;					  // The bottom of each slider, and the least that can be typed
			int32_t sliderMax = 200;				  // The top of each slider; more can be typed
			bool rangeFollowsValue = false;			  // Slide 50 either side of the value (from sliderMin)
			std::function<void(size_t, float)> onSet; // Use percentage `value` for factor `index`
		};

		/** Create the dialog, closed and with no factors */
		explicit FactorDialog(Settings settings);

		/** Declare `<id>.open`, `<id>.title` and `<id>.factors` */
		void fields(FieldVisitor& f) override;

		/** Show `count` factors, called `labelOf(i)` with percentage `valueOf(i)` */
		void show(size_t count,
				  const std::function<std::string(size_t)>& labelOf,
				  const std::function<float(size_t)>& valueOf);

		/** Open the dialog */
		void open();

		/** Close the dialog */
		void close();

		/** Use percentage `value` for factor `index`, as the user chose it */
		void set(size_t index, float value);

	  private:
		/** Show `value` for factor `row`, moving its slider's range if it follows the value */
		void setRow(FactorRow& row, int32_t value) const;

		Settings m_settings;		// How it is set up
		IntSubject m_open;			// `<id>.open`
		StringSubject<64> m_title;	// `<id>.title`
		List<FactorRow> m_factors;	// `<id>.factors`
	};
} // namespace UI::Xml
