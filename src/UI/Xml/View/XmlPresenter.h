/*
 * XmlPresenter.h
 *
 * The C++ side of an XML view. A component names its presenter on its root element:
 *
 *     <component presenter="fans"> ... </component>
 *
 * When an XmlView builds that component it first creates the presenter, then binds the names the presenter declares
 * in `fields()` into the view's scope, so the view's widgets can bind to them. `<action>`s in the view reach the
 * handlers the presenter registers with `action()`.
 *
 * A presenter writes subjects and handles actions. It never sees a widget, which keeps logic out of the XML and lets
 * users restyle or restructure a view freely. Model events are subscribed in `onInit()` as for every other presenter
 * (`registerEventListener<EventType::...>`), and are only delivered while the view is shown.
 *
 * Presenters are registered by name (see Presenters below), normally in Screens/XmlPresenters.cpp.
 */

#pragma once

#include "Actions.h"
#include "UI/Core/Presenter.h"
#include "UI/Xml/Binding/Fields.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	class XmlPresenter : public BasePresenter, public FieldProvider, public std::enable_shared_from_this<XmlPresenter>
	{
	  public:
		/** @param name The name the presenter is registered under, used in logs */
		explicit XmlPresenter(std::string name)
			: m_name(std::move(name))
		{
		}

		/** The name the presenter is registered under */
		std::string_view getName() const override { return m_name; }

		/** Declare the subjects and lists the view can bind to. The default declares none. */
		void fields(FieldVisitor&) override {}

		/** The actions this presenter handles */
		const ActionHandlerSet& getActions() const { return m_actions; }

	  protected:
		/**
		 * Handle `<action name="…">` raised in the view. Call from the constructor.
		 * @param description What the action does and what it takes (`arg`, the edited value, the row it is in), for
		 * the UI API reference
		 */
		void action(std::string name, std::string description, ActionHandler handler)
		{
			m_actions.add(std::move(name), std::move(description), std::move(handler));
		}

	  private:
		std::string m_name;			// Registered name
		ActionHandlerSet m_actions; // Handlers added with action()
	};

	/** Creates a presenter */
	using PresenterFactory = std::function<std::shared_ptr<XmlPresenter>()>;

	/** The presenters components can name. Registrations last for the life of the program. */
	namespace Presenters
	{
		/** Make `factory` available as `<component presenter="name">`, replacing any earlier one */
		void add(std::string name, PresenterFactory factory);

		/** Make presenter type P available as `<component presenter="name">` */
		template <typename P>
			requires std::derived_from<P, XmlPresenter> && std::default_initializable<P>
		void add(std::string name)
		{
			add(std::move(name), [] { return std::make_shared<P>(); });
		}

		/** @return whether a presenter called `name` is registered */
		bool exists(std::string_view name);

		/** @return a new presenter called `name`, or nullptr if there is none */
		std::shared_ptr<XmlPresenter> create(std::string_view name);

		/** The names of every registered presenter, in order */
		std::vector<std::string> names();
	} // namespace Presenters
} // namespace UI::Xml
