/*
 * SettingsPresenter.cpp
 */

#include "SettingsPresenter.h"

namespace UI
{
	SettingsPresenter::SettingsPresenter(std::string name)
		: XmlPresenter(std::move(name))
	{
	}

	void SettingsPresenter::setting(std::string name, std::string description, std::function<void(float value)> store)
	{
		action(std::move(name),
			   std::move(description),
			   [store = std::move(store)](const Xml::ActionArgs& args)
			   {
				   if (const auto value = args.value())
				   {
					   store(*value);
				   }
			   });
	}

	void SettingsPresenter::toggle(std::string name,
								   std::string description,
								   Xml::IntSubject& shown,
								   std::function<void(bool on)> apply)
	{
		setting(std::move(name),
				std::move(description),
				[&shown, apply = std::move(apply)](float value)
				{
					const bool on = value != 0.0f;
					shown.set(on ? 1 : 0);
					apply(on);
				});
	}
} // namespace UI
