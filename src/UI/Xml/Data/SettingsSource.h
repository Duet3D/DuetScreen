/*
 * SettingsSource.h
 *
 * The screen's settings, each declared once in C++ and published for every view: the settings pages, and any widget
 * that shows or changes one (a brightness slider, a dark mode switch). A setting is a number (a value, an index of its
 * options, 0 or 1 for a switch) or a text:
 *
 *     Settings::add({.name = "confirmations",
 *                    .description = "1 if actions are confirmed first",
 *                    .read = [] { return StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS) ? 1 : 0; },
 *                    .write = [](int32_t on) { StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, on != 0); }});
 *
 *     <ds_toggle><bind_state_if_eq subject="source.settings.confirmations" state="checked" ref_value="1"/>
 *         <action trigger="value_changed" name="actions.settings.set" arg="confirmations"/></ds_toggle>
 *
 * A setting with options also publishes them, translated, one per line, as `source.settings.<name>_options` (e.g. for
 * a `<ds_choice>`). One without `write` is only shown, e.g. whether the next setting applies. Settings are added
 * before the XML UI starts (src/UI/Presenters/Settings/); `actions.settings.set` and `actions.settings.ask` change them
 * (src/UI/Presenters/Settings/SettingsActions.h). After any change every setting is read again, as one may depend on
 * another.
 *
 * The names it publishes are listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** A setting whose value is a number */
	struct Setting
	{
		std::string name;								// `source.settings.<name>`, and the arg that changes it
		std::string description;						// What its value means, for the UI API reference
		std::function<int32_t()> read;					// Its value now
		std::function<void(int32_t)> write = nullptr;	// Use and keep a value the user chose; none if only shown
		std::function<std::string()> options = nullptr; // What it can be, translated, one per line; none if any number
		int32_t scale = 1; // Edited in units of this many: e.g. 1000 for ms on a slider of seconds (value_scale)
	};

	/** A setting whose value is text */
	struct TextSetting
	{
		std::string name;									  // `source.settings.<name>`, and the arg that changes it
		std::string description;							  // What it is, for the UI API reference
		std::function<std::string()> read;					  // Its value now
		std::function<void(std::string_view)> write;		  // Use and keep a value the user chose
		std::function<std::string()> askHeader = nullptr;	  // Asked for with this header by actions.settings.ask
	};

	/** The settings there are. Added for the life of the program, before the XML UI starts. */
	namespace Settings
	{
		/** Add a setting whose value is a number */
		void add(Setting setting);

		/** Add a setting whose value is text */
		void add(TextSetting setting);

		/** `items` one per line, as a setting's options are written */
		std::string lines(const std::vector<std::string>& items);
	} // namespace Settings

	/** Publishes `source.settings.*` */
	class SettingsSource : public DataSource
	{
	  public:
		/** Create the source, with a subject for every setting added. Nothing is published until DataSources::start(). */
		SettingsSource();

		/** Declare every setting, and the options of those that have them */
		void fields(FieldVisitor& f) override;

		/** Read every setting again */
		void update() override;

		/** Settings don't depend on the printer: read them again */
		void clear() override { update(); }

		/**
		 * Use `edited` for the number setting `name`, in its edited units (see Setting::scale); false if there is no
		 * such setting the user can change
		 */
		bool set(std::string_view name, float edited);

		/** Use `value` for the text setting `name`; false if there is no such setting */
		bool set(std::string_view name, std::string_view value);

		/** The text setting called `name`, or nullptr */
		const TextSetting* textSetting(std::string_view name) const;

	  protected:
		/** Read the settings again when the theme changes, as several follow it */
		void onInit() override;

	  private:
		/** A number setting, published */
		struct Number
		{
			const Setting* setting;		  // What it is
			IntSubject value;			  // `<name>`
			StringSubject<1024> options;  // `<name>_options`
		};

		/** A text setting, published */
		struct Text
		{
			const TextSetting* setting; // What it is
			StringSubject<256> value;	// `<name>`
		};

		std::deque<Number> m_numbers; // Every number setting; a deque, so the subjects never move
		std::deque<Text> m_texts;	  // Every text setting
	};
} // namespace UI::Xml
