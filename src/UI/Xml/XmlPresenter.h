/*
 * XmlPresenter.h
 *
 * The C++ side of an XML view. A component names its presenter on its root element:
 *
 *     <component presenter="fans"> ... </component>
 *
 * When an XmlView builds that component it creates the registered presenter first, so the subjects the presenter
 * adds (with `subject()`) are visible to the view's bindings, and the actions it adds (with `action()`) are what the
 * view's `<action>` elements reach.
 *
 * A presenter writes subjects and handles actions. It never receives a view or a widget, which keeps logic out of
 * the XML and lets users restyle or restructure the view freely. Model events are subscribed in `onInit()` exactly as
 * for the C++ presenters (`registerEventListener<EventType::...>`).
 */

#pragma once

#include "UI/Core/Presenter.h"
#include "XmlActions.h"
#include "XmlSubjects.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace UI::Xml
{
	class XmlPresenter : public BasePresenter
	{
	  public:
		explicit XmlPresenter(std::string name)
			: m_name(std::move(name))
		{
		}

		std::string_view getName() const override { return m_name; }

		lv_xml_subject_ctx_t* getContext() const { return m_ctx.get(); }
		const ActionHandlerSet& getActions() const { return m_actions; }

	  protected:
		/** Make `subject` available to the view's bindings as `name`. Call from the constructor. */
		template <typename S>
		void subject(const std::string& name, S& subject)
		{
			m_ctx.add(name, subject);
		}

		/** Handle `<action name="…">` raised in the view. Call from the constructor. */
		void action(std::string name, ActionHandler handler) { m_actions.add(std::move(name), std::move(handler)); }

	  private:
		std::string m_name;
		SubjectContext m_ctx;
		ActionHandlerSet m_actions;
	};

	using PresenterFactory = std::function<std::shared_ptr<XmlPresenter>()>;

	namespace Presenters
	{
		void add(std::string name, PresenterFactory factory);
		bool exists(std::string_view name);
		/** @return the new presenter, or nullptr if no presenter of that name is registered */
		std::shared_ptr<XmlPresenter> create(std::string_view name);
		void clear();

		template <typename P>
			requires std::derived_from<P, XmlPresenter>
		void add(std::string name)
		{
			add(std::move(name), [] { return std::make_shared<P>(); });
		}
	} // namespace Presenters
} // namespace UI::Xml
