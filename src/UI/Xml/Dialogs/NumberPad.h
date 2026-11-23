/*
 * NumberPad.h
 *
 * The state behind a number pad (assets/ui/components/dialogs/numpad.xml): what has been typed on its keys, the numbers
 * that can be, and whether what is typed can be used. Its keys type as a calculator's do: the first replaces the value
 * the pad starts with; backspace takes one character; nothing typed is 0.
 *
 * A presenter owns one per pad it shows, under an id that prefixes its fields and actions, and declares its fields:
 *
 *     MyPresenter::MyPresenter() : XmlPresenter("my"), m_pad(*this, "pad") {}
 *     void MyPresenter::fields(FieldVisitor& f) { m_pad.fields(f); }
 *
 *     <numpad pad="pad"/>
 *
 * Its fields are `<id>.text`, `<id>.range`, `<id>.confirmable`, `<id>.signed`, `<id>.decimal` and `<id>.steppable`;
 * its actions `<id>.key`, `<id>.backspace`, `<id>.sign`, `<id>.clear` and `<id>.step` (− and + beside the readout,
 * given a step with setStep()). Both are listed in the UI API reference
 * (docs/XML_UI_API.md). The number prompt (PromptPresenters.h) and the printer's alerts asking for a number use one.
 */

#pragma once

#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/Subjects.h"
#include <functional>
#include <string>
#include <string_view>

namespace UI::Xml
{
	class XmlPresenter;

	class NumberPad : public FieldProvider
	{
	  public:
		/** A pad asking for nothing yet, its actions added to `owner` under `id` */
		NumberPad(XmlPresenter& owner, std::string id);

		/** Declare the pad's fields under its id */
		void fields(FieldVisitor& f) override;

		/**
		 * Ask for a number from `min` to `max` (whole if `whole`), starting at `start` (empty if not set), in `unit`
		 * (shown after it and its limits, e.g. "s"; empty for none)
		 */
		void askNumber(float min, float max, std::optional<float> start, bool whole = false, std::string_view unit = {});

		/** Ask for digits and dots, e.g. an IP address, starting with `start` */
		void askText(std::string_view start);

		/** Let − and + take `step` off the number or add it, within the limits; 0 for no − and + */
		void setStep(float step);

		/** Show `value`, as a value started with is: the next key typed replaces it, e.g. a preset tapped */
		void put(float value);

		/** Call `callback` whenever what is typed changes */
		void setOnChange(std::function<void()> callback) { m_onChange = std::move(callback); }

		/** What has been typed */
		std::string_view text() const { return m_typed.get(); }

		/** What has been typed, as a number: nothing typed (or only a sign or a dot) is 0 */
		float value() const;

		/** Whether what is typed can be used: a number within the limits, or any digits and dots */
		bool confirmable() const;

		/**
		 * The numbers that can be typed from `min` to `max`, in `unit` if it isn't empty, e.g. "0 to 280" or "0 s or
		 * more"; empty for any
		 */
		static std::string rangeOf(float min, float max, std::string_view unit = {});

		/** `value` as the user reads it, e.g. "0.1" for 0.1f rather than "0.100000001" */
		static std::string shown(float value);

	  private:
		/** Type `key`: a digit or a dot */
		void type(char key);

		/** Show `text` as typed, and whether it can be used */
		void setText(std::string_view text);

		std::string m_id;					// Prefix of its fields and actions
		bool m_number = false;				// A number is asked for, rather than digits and dots
		float m_min = 0.0f;					// The least number that can be typed
		float m_max = 0.0f;					// The most number that can be typed
		bool m_fresh = false;				// Showing the value it started with: a key replaces it
		float m_step = 0.0f;				// What − and + take off or add; 0 for none
		std::function<void()> m_onChange;	// Told when what is typed changes

		StringSubject<32> m_typed;	// `<id>.text`
		StringSubject<64> m_range;	// `<id>.range`
		StringSubject<16> m_unit;	// `<id>.unit`: the number's unit, or empty
		IntSubject m_confirmable;	// `<id>.confirmable`
		IntSubject m_signed;		// `<id>.signed`: 1 if the number may be negative
		IntSubject m_decimal{1};	// `<id>.decimal`: 1 if the number may have a fraction
		IntSubject m_steppable;		// `<id>.steppable`: 1 if − and + step the number
	};
} // namespace UI::Xml
