/*
 * NumberEntry.h
 *
 * The fields and actions behind numberpad_panel.xml: a header, a box showing what has been typed, a clear button and
 * the number keys. A presenter owns one NumberEntry per panel, declares its fields and handles its actions, as with a
 * ValueSelector:
 *
 *     MyPresenter::MyPresenter() : XmlPresenter("my")
 *     {
 *         for (auto& [name, description, handler] : Xml::NumberEntry::actions({&m_pad}))
 *             action(name, description, handler);
 *     }
 *     void MyPresenter::fields(Xml::FieldVisitor& f) { m_pad.fields(f); }
 *
 *     <numberpad_panel entry="pad" header="pad.header" text="pad.text" accepted="pad.accepted"
 *                      confirmable="pad.confirmable" signed="pad.signed"/>
 *
 * Its fields are named under the entry's id (e.g. "pad.text"). They are declared in fields(), and its actions in
 * actions(); both are listed in the UI API reference (docs/XML_UI_API.md).
 *
 * Most code doesn't own one: it asks the shared number pad with Services::askNumber() or askText().
 */

#pragma once

#include "Actions.h"
#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/Subjects.h"
#include "lvgl/lvgl.h"
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** A number to ask the user for */
	struct NumberRequest
	{
		std::string header;					  // Shown above the keys, e.g. "T0 heater 1 active"; none if empty
		float min = 0.0f;					  // Smallest value accepted; below 0, the +/- key works
		float max = 0.0f;					  // Largest value accepted
		std::optional<float> value;			  // Shown to start with; the box starts empty if not set
		std::function<void(float)> onConfirm; // Called with the value the user confirms
		lv_obj_t* asker = nullptr;			  // The widget asking, if the request must go with it (see NumberEntry)
	};

	/** Text typed on the number keys, such as an IP address: digits and dots */
	struct TextRequest
	{
		std::string header;								 // Shown above the keys
		std::string text;								 // Shown to start with
		std::function<void(std::string_view)> onConfirm; // Called with the text the user confirms
	};

	class NumberEntry : public FieldProvider
	{
	  public:
		/** Create an entry whose fields are named under `id`, asking for nothing */
		explicit NumberEntry(std::string id);

		/** Forget the asker */
		~NumberEntry() override;

		NumberEntry(const NumberEntry&) = delete;
		NumberEntry& operator=(const NumberEntry&) = delete;

		/** Declare `<id>.header`, `<id>.text`, `<id>.accepted`, `<id>.confirmable` and `<id>.signed` */
		void fields(FieldVisitor& f) override;

		/**
		 * Ask for a number, replacing any earlier request. If `request.asker` is set and is deleted before the user
		 * confirms, the request is dropped and onDropped() runs, so its callback never reaches a deleted widget.
		 */
		void ask(NumberRequest request);

		/** Ask for text typed on the number keys, replacing any earlier request */
		void ask(TextRequest request);

		/** Show `value` in the box, e.g. a slider's as it is dragged, keeping the request */
		void showValue(float value);

		/** Drop the request: confirming does nothing until the next one */
		void drop();

		/** Run `callback` when the user confirms a value, before it is passed on, e.g. to close the dialog showing it */
		void setOnConfirmed(std::function<void()> callback) { m_onConfirmed = std::move(callback); }

		/** Run `callback` when the request is dropped because its asker was deleted, e.g. to close the dialog */
		void setOnDropped(std::function<void()> callback) { m_onDropped = std::move(callback); }

		/** One entry action, to register with XmlPresenter::action() */
		struct Action
		{
			std::string name;		 // Action name, e.g. "numberpad.confirm"
			std::string description; // What it does, for the UI API reference
			ActionHandler handler;	 // Runs it
		};

		/** The entry actions (see the file comment) of any of `entries`, to register */
		static std::vector<Action> actions(std::initializer_list<NumberEntry*> entries);

	  private:
		/** Show `text` as typed, and whether it can be confirmed */
		void setText(std::string_view text);

		/** Whether what is typed can be confirmed: a number within the limits, or any text */
		bool confirmable() const;

		/** Confirm what is typed, if it can be */
		void confirm();

		/** Watch `asker` (or nothing), dropping the request if it is deleted */
		void watchAsker(lv_obj_t* asker);

		/** The asker was deleted: drop its request */
		static void askerDeletedCb(lv_event_t* e);

		std::string m_id; // Prefix of its fields, and its actions' arg

		std::optional<NumberRequest> m_number; // The number asked for, if any
		std::optional<TextRequest> m_text;	   // The text asked for, if any
		lv_obj_t* m_asker = nullptr;		   // The widget that must outlive m_number, if any

		std::function<void()> m_onConfirmed; // Run after a confirm
		std::function<void()> m_onDropped;	 // Run when the asker goes

		StringSubject<128> m_header;	// `<id>.header`
		StringSubject<32> m_typed;		// `<id>.text`
		StringSubject<16> m_accepted;	// `<id>.accepted`: the characters the box takes
		IntSubject m_confirmable;		// `<id>.confirmable`
		IntSubject m_signed;			// `<id>.signed`: 1 if the value may be negative
	};
} // namespace UI::Xml
