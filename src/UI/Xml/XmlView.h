/*
 * XmlView.h
 *
 * Hosts an XML component as an ordinary LvObj, so it can be used anywhere a C++ view is (tabs, navigation, the app
 * drawer). The component's root object *is* this LvObj's root, so the widget tree is the same shape as a C++ view's.
 *
 * Construction order matters and is enforced by the private base: the presenter named by the component is created
 * first and adds its subjects to the view's context, then the component is built with that context active.
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include "XmlPresenter.h"
#include <memory>
#include <string>

namespace UI::Xml
{
	namespace detail
	{
		struct XmlViewState
		{
			explicit XmlViewState(std::string componentName, std::string objName);

			lv_xml_subject_ctx_t* context() const;

			std::string component;
			std::string objName;
			std::shared_ptr<XmlPresenter> presenter;
			/** Used when the component has no presenter */
			SubjectContext emptyContext;
		};
	} // namespace detail

	class XmlView : private detail::XmlViewState, public LvObj
	{
	  public:
		XmlView(const std::string& name, LvObj& parent, std::string component);
		~XmlView() override;

		const std::string& getComponent() const { return component; }

		/** The presenter the component named, or nullptr */
		XmlPresenter* getPresenter() const { return presenter.get(); }

		/**
		 * Rebuild from the component's current definition, e.g. after a bundle reload. The presenter and its subjects
		 * are kept, so the new widgets come up showing the current state.
		 */
		void rebuild();

	  protected:
		void showInner() override;
		void hideInner() override;

	  private:
		static lv_obj_t* build(lv_obj_t* parent, detail::XmlViewState& state);
		void registerActions();
	};
} // namespace UI::Xml
