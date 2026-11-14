/*
 * SettingsPresenter.h
 *
 * What the presenters of the settings pages share: registering an action that stores the value its widget gives,
 * and one that stores a switch's on or off and shows it.
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <functional>
#include <string>

namespace UI
{
	class SettingsPresenter : public Xml::XmlPresenter
	{
	  protected:
		/** A presenter registered as `name` */
		explicit SettingsPresenter(std::string name);

		/**
		 * Register action `name`, described by `description`, that hands the value the widget gives (its edited
		 * value, or the action's `arg`) to `store`; it does nothing without a value
		 */
		void setting(std::string name, std::string description, std::function<void(float value)> store);

		/**
		 * Register action `name`, described by `description`, that hands a switch's on (edited value 1) or off (0) to
		 * `apply`, and shows it in `shown`
		 */
		void toggle(std::string name, std::string description, Xml::IntSubject& shown, std::function<void(bool on)> apply);
	};
} // namespace UI
