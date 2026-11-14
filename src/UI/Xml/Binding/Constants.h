/*
 * Constants.h
 *
 * Wherever XML binds a name, it may give a constant instead, so a component whose prop takes a name (a gauge's
 * `max`, its `title`) can be given a fixed value just as well as data:
 *
 *     100          a whole number: an int subject holding 100
 *     'Hotend'     text between single quotes: a text subject holding what is between them
 *     tr.<key>     a translation key: a text subject holding the translation, which follows the language
 *
 *     <gauge_bar title="tr.widget.job_progress" value="source.job.progress" max="100" unit="'%'"/>
 *
 * A constant is made the first time it is used, then shared and kept until the XML engine stops. The globals answer
 * constants (see Globals.h), after the names they hold and before their resolver, so a constant binds anywhere.
 */

#pragma once

#include <optional>
#include <string_view>

namespace UI::Xml
{
	class Subject;

	namespace Constants
	{
		/** What a constant is */
		enum class Kind
		{
			number,		 // A whole number, e.g. `100`
			text,		 // Text between single quotes, e.g. `'Hotend'`
			translation, // A translation key, `tr.<key>`
		};

		/** What `name` is as a constant, or nothing if it isn't one */
		std::optional<Kind> kindOf(std::string_view name);

		/** The subject of the constant `name`, made the first time it is asked for, or nullptr if it isn't one */
		Subject* find(std::string_view name);

		/** Translate the translation constants again (after a language change) */
		void retranslate();

		/** Forget every constant (when the XML engine stops, after every view is gone) */
		void clear();
	} // namespace Constants
} // namespace UI::Xml
