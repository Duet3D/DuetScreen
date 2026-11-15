/*
 * FactorDialog.h
 *
 * The fields behind factors_dialog.xml: a dialog of percentages, such as the speed factor or each extruder's
 * extrusion factor, each with its value large (tap it to type it: the number pad is the dialog's next step), a slider,
 * and a row of quick values (its settings' presets), the one in use marked.
 * Its owner (FactorsSource) fills in its factors and declares its fields, named under the dialog's id:
 *
 *     void FactorsSource::fields(FieldVisitor& f) { m_speed.fields(f); }  // source.factors.speed.open, ...
 *
 *     <factors_dialog open="source.factors.speed.open" title="source.factors.speed.title"
 *                     factors="source.factors.speed.factors"/>
 *
 * The global actions `actions.factors.set`, raised from a row, which knows its dialog, and `actions.factors.preset`,
 * raised from a row of a row's `presets`, set a factor (src/UI/Presenters/Modals/FactorsActions.h). The dialog closes itself when the user closes it (DsDialog.h).
 */

#pragma once

#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	class FactorDialog;
	class FactorRow;

	/** A quick value of a FactorRow, e.g. 100% */
	class FactorPresetRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<16> text;		   // E.g. "100%"
		IntSubject chosen;			   // 1 if it is the factor's value
		int32_t percent = 0;		   // The value it sets; not shown
		FactorRow* factor = nullptr;   // The factor it sets; not shown
	};

	/** One percentage of a FactorDialog */
	class FactorRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<32> label;		 // What it is, e.g. "Extruder 0"; empty if the dialog's title says it
		StringSubject<96> detail;		 // More about it, e.g. the tools that use the extruder
		IntSubject value;				 // The percentage, e.g. 100
		StringSubject<16> text;			 // The percentage as shown, e.g. "100%"
		IntSubject min;					 // The bottom of its slider
		IntSubject max;					 // The top of its slider
		List<FactorPresetRow> presets;	 // Its quick values
		FactorDialog* owner = nullptr;	 // The dialog it belongs to; not shown
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
			std::vector<int32_t> presets;			  // Each factor's quick values, e.g. {90, 95, 100, 105, 110}
			std::function<void(size_t, float)> onSet; // Use percentage `value` for factor `index`
		};

		/** Create the dialog, closed and with no factors */
		explicit FactorDialog(Settings settings);

		/** Declare `<id>.open`, `<id>.title` and `<id>.factors` */
		void fields(FieldVisitor& f) override;

		/** Show `count` factors, called `labelOf(i)`, described by `detailOf(i)` if given, with percentage `valueOf(i)` */
		void show(size_t count,
				  const std::function<std::string(size_t)>& labelOf,
				  const std::function<float(size_t)>& valueOf,
				  const std::function<std::string(size_t)>& detailOf = {});

		/** Show `title` above the factors */
		void setTitle(std::string_view title);

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
