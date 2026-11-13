/*
 * ApiLint.h
 *
 * Checks the loaded components against the UI API reference (ApiReference.h): every name a component uses must be
 * one the UI provides where the component is used. Without this a typo is silent: an unknown subject leaves a label
 * empty, an unknown action does nothing until tapped, an unknown style is ignored.
 *
 * What is checked, in each component's `<view>`:
 *   - element names: a widget (`lv_*`, `ds_*`), a component, a binding (`bind_*`) or `<action>`, `<style>`, …
 *   - subjects and lists: `bind_*` and `subject` attributes, the subject attributes of DuetScreen widgets (see
 *     AttributeKind), the operands of `cond=` expressions and the `{name}` placeholders of an action's `arg`
 *   - actions and their triggers, styles (theme or the component's own `<styles>`), fonts, `#consts`, translation
 *     keys, components, C++ views and presenters
 *
 * A name is looked up where the component is used, as at runtime. A component that nothing uses is a view of its own
 * (shown from C++): its names are its presenter's, then the globals. `<ds_view view="x">` starts a new view, with
 * the presenter it names, or else x's own. `<ds_list list="l" item="x">` builds x in a row of l, so l's row fields
 * come first. A component used as an element is built in the scope it is written in, with its `$props` replaced by
 * the values given there. Actions are looked for in the presenters of every enclosing view, then the global actions.
 * A component used in several places is checked in each.
 *
 * Attributes themselves are not checked: whether `<lv_label long_mode>` exists is the XML engine's to say, and it
 * warns about unknown ones as it builds the view.
 */

#pragma once

#include "ApiReference.h"
#include <string>
#include <vector>

namespace UI::Xml::Api
{
	/** A name a component uses that the UI doesn't provide there */
	struct LintIssue
	{
		std::string file;	 // The component's file
		int line = 0;		 // Line of the element that uses the name
		std::string message; // What is wrong, and where the component was used, e.g. "in fan_screen › fans"

		/** Equal if every member is */
		bool operator==(const LintIssue&) const = default;
	};

	/**
	 * Check every component `reference` lists against it. Requires Xml::init(): element names are checked against
	 * the XML engine's registrations.
	 * @return the problems found, in file and line order, each once
	 */
	std::vector<LintIssue> lint(const Reference& reference);

	/** `issue` as "file:line: message" */
	std::string toString(const LintIssue& issue);
} // namespace UI::Xml::Api
