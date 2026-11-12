/*
 * XmlView.h
 *
 * Hosts an XML component as an ordinary LvObj, so it can be used anywhere a C++ view is (tabs, navigation, the app
 * drawer). The component's root widget *is* this LvObj's root, so the widget tree has the same shape as a C++ view's.
 *
 * The view owns the component's presenter (if it names one) and a top-level scope holding the presenter's fields.
 * Like a C++ View, it activates the presenter and connects it to the Model while shown, and disconnects it while
 * hidden.
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include "UI/Xml/Binding/Scope.h"
#include "XmlPresenter.h"
#include <memory>
#include <string>

namespace UI::Xml
{
	namespace detail
	{
		/**
		 * The parts of an XmlView that must exist before its widgets are built. XmlView inherits this *before* LvObj,
		 * whose constructor builds the widgets, so C++ construction order guarantees the presenter and scope are
		 * ready in time.
		 */
		struct XmlViewState
		{
			/** Create the presenter `component` names, and a scope holding its fields */
			XmlViewState(std::string component, std::string objName);

			/** (Re)create the presenter if the component's definition names a different one, and a fresh scope */
			void prepare();

			std::string component;					// Name of the component shown
			std::string objName;					// Name given to the root widget
			std::shared_ptr<XmlPresenter> presenter; // The component's presenter, or nullptr if it names none
			std::unique_ptr<Scope> scope;			// Names the component's widgets bind to
		};
	} // namespace detail

	class XmlView : private detail::XmlViewState, public LvObj
	{
	  public:
		/**
		 * Build `component` inside `parent`
		 * @param name Name of the root widget
		 * @param parent Parent view
		 * @param component Name of a loaded XML component
		 */
		XmlView(const std::string& name, LvObj& parent, std::string component);

		/** Build `component` inside a widget that has no C++ view, e.g. one created from XML */
		XmlView(const std::string& name, lv_obj_t* parent, std::string component);

		/** Disconnect the presenter; the widgets are deleted by ~LvObj */
		~XmlView() override;

		/** Name of the component shown */
		const std::string& getComponent() const { return component; }

		/** The presenter the component names, or nullptr */
		XmlPresenter* getPresenter() const { return presenter.get(); }

		/**
		 * Rebuild from the component's current definition, e.g. after a bundle reload. The presenter, and so the
		 * current state, is kept unless the new definition names a different one.
		 */
		void rebuild();

	  protected:
		/** Activate the presenter and connect it to the Model, then show */
		void showInner() override;

		/** Deactivate the presenter and disconnect it from the Model, then hide */
		void hideInner() override;

	  private:
		/** Build the component inside `parent`, or a placeholder saying it couldn't be loaded */
		static lv_obj_t* build(lv_obj_t* parent, detail::XmlViewState& state);

		/** Point actions raised inside the view at the presenter */
		void registerActions();

		/** Connect the presenter to the Model and activate it */
		void connectPresenter();

		/** Deactivate the presenter and disconnect it from the Model */
		void disconnectPresenter();
	};
} // namespace UI::Xml
