#pragma once

#include "View.h"

#include <concepts>
#include <string>
#include <string_view>
#include <vector>

namespace UI
{
	void back();
	void home();

	/**
	 * What navigation needs of the dialog surface (UI/Xml/Dialogs/DialogHost.h): the back key goes back through its
	 * steps first, and opening or closing a screen closes its dialog
	 */
	class DialogLayer
	{
	  public:
		virtual ~DialogLayer() = default;

		/** The back key: go back a step, or close the dialog; false if no dialog is open */
		virtual bool back() = 0;

		/** Close the user's dialog, e.g. when the page changes */
		virtual void closeAll() = 0;

		/** Close the steps above the highest one `obj` is in, or every step if it is in none */
		virtual void closeNotAround(lv_obj_t* obj) = 0;

		/** Whether a dialog is open */
		virtual bool isOpen() const = 0;
	};

	/** Make `layer` the dialogs navigation goes through (nullptr for none) */
	void setDialogLayer(DialogLayer* layer);

	/** A dialog opened or closed: update what depends on it, such as the back button */
	void dialogsChanged();

	/** Close the user's dialog, as opening a screen does */
	void closeDialogs();

	/**
	 * Close the dialog's steps that `obj` isn't in, e.g. when a tab view changes tab: a dialog opened from the tab left
	 * behind goes with it, but not a dialog the tab view is in
	 */
	void closeDialogsNotAround(lv_obj_t* obj);

	/*
	 * Named views. A view registered by name can be found, and opened, by that name instead of through whatever owns
	 * it. This is how the XML UI's `<ds_view>` makes the views it hosts available, e.g. `findView<XmlView>
	 * ("numberpad")` or `<action name="actions.navigation.open" arg="control"/>`.
	 */

	/** Make `view` findable as `name`, replacing any earlier view of that name */
	void registerView(std::string name, LvObj* view);

	/** Forget `view`: its name and any home, open or returnable entry. Call before destroying a view. */
	void forgetView(LvObj* view);

	/** The view registered as `name`, or nullptr */
	LvObj* findView(std::string_view name);

	/** The view registered as `name` if it is a T, or nullptr */
	template <typename T>
	T* findView(std::string_view name)
	{
		return dynamic_cast<T*>(findView(name));
	}

	/** The name of the open screen (the top one), or empty when the home screens are showing */
	std::string_view getCurrentScreenName();

	/** The name `view` is registered under, or empty */
	std::string_view getViewName(const LvObj* view);

	/** Open the screen registered as `name` (see openScreen()). @return false if there is no such view */
	bool openScreen(std::string_view name, bool closePrevious = true);

	/** Where navigation is, by view name, so it survives the views being rebuilt (see saveNavigationState()) */
	struct NavigationState
	{
		std::vector<std::string> open;		 // The open screens, the current one last
		std::vector<std::string> returnable; // The screens the back button returns to, the next one last
	};

	/** Where navigation is now. Screens with no name are left out. */
	NavigationState saveNavigationState();

	/**
	 * Go back to `state`, e.g. once a reload has rebuilt the views: open its screens again, and let the back button
	 * return to its returnable ones. Unlike openScreen(), an open dialog is left alone and nothing fades in. Names with
	 * no view any more are skipped.
	 */
	void restoreNavigationState(const NavigationState& state);

	// Internal helpers used by the template APIs
	namespace detail
	{
		void addHomeScreenImpl(LvObj* view);
		void removeHomeScreenImpl(LvObj* view, bool close);
		void openScreenImpl(LvObj* view, bool closePrevious);
		bool closeScreenImpl(LvObj* view, bool returnable);
	} // namespace detail

	template <std::derived_from<LvObj> T>
	void addHomeScreen(T* view)
	{
		detail::addHomeScreenImpl(static_cast<LvObj*>(view));
	}

	template <std::derived_from<LvObj> T>
	void removeHomeScreen(T* view, bool close = true)
	{
		detail::removeHomeScreenImpl(static_cast<LvObj*>(view), close);
	}

	void clearHomeScreens();

	template <std::derived_from<LvObj> T>
	void openScreen(T* view, bool closePrevious = true)
	{
		detail::openScreenImpl(static_cast<LvObj*>(view), closePrevious);
	}

	template <std::derived_from<LvObj> T>
	bool closeScreen(T* view, bool returnable = true)
	{
		return detail::closeScreenImpl(static_cast<LvObj*>(view), returnable);
	}

	LvObj* getCurrentScreen();
	bool closeLastScreen();
} // namespace UI
