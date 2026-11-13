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

	/*
	 * Named views. A view registered by name can be found, and opened, by that name instead of through whatever owns
	 * it. This is how the XML UI's `<ds_view>` makes the views it hosts available, e.g. `findView<ModalNumberPad>
	 * ("numberpad")` or `<action name="nav.open" arg="control"/>`.
	 */

	/** Make `view` findable as `name`, replacing any earlier view of that name */
	void registerView(std::string name, LvObj* view);

	/** Forget `view`: its name and any home, open, returnable or modal entry. Call before destroying a view. */
	void forgetView(LvObj* view);

	/** The view registered as `name`, or nullptr */
	LvObj* findView(std::string_view name);

	/** The view registered as `name` if it is a T, or nullptr */
	template <typename T>
	T* findView(std::string_view name)
	{
		return dynamic_cast<T*>(findView(name));
	}

	/** Every registered view */
	std::vector<LvObj*> getViews();

	/** Every registered view that is a T */
	template <typename T>
	std::vector<T*> findViews()
	{
		std::vector<T*> found;
		for (LvObj* view : getViews())
		{
			if (auto* t = dynamic_cast<T*>(view))
			{
				found.push_back(t);
			}
		}
		return found;
	}

	/** The name of the open screen (the top one), or empty when the home screens are showing */
	std::string_view getCurrentScreenName();

	/** The name `view` is registered under, or empty */
	std::string_view getViewName(const LvObj* view);

	/**
	 * Where modals belong: the view registered as `main_window` (the area beside the sidebar and below the status
	 * bar), so their backdrop dims the page and not the navigation. nullptr if there is none.
	 */
	LvObj* getModalHost();

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
	 * return to its returnable ones. Unlike openScreen(), open modals are left alone and nothing fades in. Names with
	 * no view any more are skipped.
	 */
	void restoreNavigationState(const NavigationState& state);

	// Internal helpers used by template APIs; not part of public modal type contract
	namespace detail
	{
		void openModalImpl(LvObj* view);
		bool closeModalImpl(LvObj* view);
		void addHomeScreenImpl(LvObj* view);
		void removeHomeScreenImpl(LvObj* view, bool close);
		void openScreenImpl(LvObj* view, bool closePrevious);
		bool closeScreenImpl(LvObj* view, bool returnable);
	} // namespace detail

	template <typename T>
	concept NotModalType = std::derived_from<T, LvObj> && !requires { typename T::modal_base_marker; };

	template <typename T>
	concept ModalType = std::derived_from<T, LvObj> && requires { typename T::modal_base_marker; };

	template <NotModalType T>
	void addHomeScreen(T* view)
	{
		detail::addHomeScreenImpl(static_cast<LvObj*>(view));
	}

	template <NotModalType T>
	void removeHomeScreen(T* view, bool close = true)
	{
		detail::removeHomeScreenImpl(static_cast<LvObj*>(view), close);
	}

	void clearHomeScreens();

	template <NotModalType T>
	void openScreen(T* view, bool closePrevious = true)
	{
		detail::openScreenImpl(static_cast<LvObj*>(view), closePrevious);
	}

	template <NotModalType T>
	bool closeScreen(T* view, bool returnable = true)
	{
		return detail::closeScreenImpl(static_cast<LvObj*>(view), returnable);
	}

	LvObj* getCurrentScreen();
	bool closeLastScreen();

	/* Modal-only APIs: only accept types deriving from Modal<...> */

	template <ModalType T>
	void openModal(T* view)
	{
		detail::openModalImpl(static_cast<LvObj*>(view));
	}

	template <ModalType T>
	bool closeModal(T* view)
	{
		return detail::closeModalImpl(static_cast<LvObj*>(view));
	}

	void closeAllModals();
	bool closeLastModal();
} // namespace UI
