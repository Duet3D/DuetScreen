#include "Navigation.h"
#include "Debug.h"
#include "UI/Components/LVGL/Transitions.h"
#include <algorithm>
#include <vector>

namespace UI
{
	using LvObjPtr = LvObj*;
	using ViewList_t = std::vector<LvObjPtr>;

	static ViewList_t s_homeScreens;
	static ViewList_t s_openScreens;
	static ViewList_t s_returnableScreens;
	static ViewList_t s_openModals;

	static bool inVector(ViewList_t& vec, LvObjPtr item)
	{
		ZoneScoped;
		return std::find(vec.begin(), vec.end(), item) != vec.end();
	}

	static bool removeFromVector(ViewList_t& vec, LvObjPtr item)
	{
		ZoneScoped;
		auto it = std::find(vec.begin(), vec.end(), item);
		if (it != vec.end())
		{
			LOG_DBG("Removing screen '{:s}' from vector", item->getName());
			vec.erase(it);
			return true;
		}
		return false;
	}

	static bool addToVector(ViewList_t& vec, LvObjPtr item)
	{
		ZoneScoped;
		removeFromVector(vec, item);
		LOG_DBG("Adding screen {:s} to vector", item->getName());
		vec.push_back(item);
		return true;
	}

	static void notifySideBar()
	{
		ZoneScoped;
		Model::get().post<EventType::NavigationHomeEnable>(s_openScreens.size() > 0);
		Model::get().post<EventType::NavigationBackEnable>(s_openScreens.size() > 0 || s_openModals.size() > 0);
		LvObj* currentScreen = s_openScreens.empty() ? nullptr : s_openScreens.back();
		Model::get().post<EventType::NavigationCurrentScreen>(currentScreen);
	}

	/**
	 * @brief Handles the action of the back button being pressed.
	 *
	 * This function performs the following steps:
	 *
	 * 1. Logs the back button press event.
	 *
	 * 2. Retrieves the current screen. If there is no current screen, logs a warning and returns.
	 *
	 * 3. If the current screen handles the back action, returns.
	 *
	 * 4. If there are open screens, closes the last open screen.
	 *
	 * 5. If there are returnable screens, opens the last returnable screen.
	 */
	void back()
	{
		ZoneScoped;
		UI_LOCK();
		LOG_INFO("Back button pressed");

		if (closeLastModal())
		{
			return;
		}

		LvObjPtr currentScreen = getCurrentScreen();
		if (currentScreen == nullptr)
		{
			LOG_WARN("No current screen");
			return;
		}
		if (!currentScreen->back())
		{
			closeScreen(currentScreen, false);

			if (s_openScreens.empty() && !s_returnableScreens.empty())
			{
				LvObjPtr lastReturnable = s_returnableScreens.back();
				openScreen(lastReturnable);
			}
		}
	}

	/**
	 * @brief Show all home screens and hide all other screens.
	 *
	 * Clears the open screens and returnable screens lists.
	 */
	void home()
	{
		ZoneScoped;
		UI_LOCK();
		LOG_INFO("Returning to home screen");
		closeAllModals();

		if (!s_openScreens.empty())
		{
			for (auto screen : s_openScreens)
			{
				if (!screen->isVisible())
				{
					continue;
				}

				// This will call screen->show() for home screens on closing last screen
				closeScreen(screen, false);
			}
		}
		else // prevents double-showing home screens
		{
			for (auto screen : s_homeScreens)
			{
				openScreen(screen);
			}
		}

		s_openModals.clear();
		s_openScreens.clear();
		s_returnableScreens.clear();
		notifySideBar();
	}

	/**
	 * @brief Clears all home screens.
	 */
	void clearHomeScreens()
	{
		ZoneScoped;
		UI_LOCK();
		s_homeScreens.clear();
	}

	/**
	 * @brief Retrieves the current screen from the list of open screens.
	 *
	 * This function checks if there are any open screens in the list. If the list
	 * is empty, it logs a warning message and returns nullptr. Otherwise, it returns
	 * the last screen in the list.
	 *
	 * @return LvObjPtr The current screen if available, otherwise nullptr.
	 */
	LvObjPtr getCurrentScreen()
	{
		ZoneScoped;
		UI_LOCK();
		if (s_openScreens.empty())
		{
			LOG_WARN("No current screen");
			return nullptr;
		}
		return s_openScreens.back();
	}

	/**
	 * @brief Closes the last visible screen.
	 *
	 * This function iterates through the list of home screens and closes any visible screen.
	 * If there are no visible home screens, it closes the last screen in the list of open screens.
	 * If the list of open screens is empty, the function returns without performing any action.
	 */
	bool closeLastScreen()
	{
		ZoneScoped;
		UI_LOCK();
		LOG_INFO("Closing last screen");
#if 0
		for (auto screen : s_homeScreens)
		{
			if (!screen->isVisible())
			{
				continue;
			}
			closeScreen(screen);
		}
#endif
		if (s_openScreens.empty())
		{
			return false;
		}
		closeScreen(s_openScreens.back());
		return true;
	}

	namespace detail
	{
		/**
		 * @brief Adds a view to the list of home screens.
		 *
		 * @param view The view to be added.
		 */
		void addHomeScreenImpl(LvObjPtr view)
		{
			ZoneScoped;
			UI_LOCK();
			if (view == nullptr)
			{
				return;
			}
			if (inVector(s_homeScreens, view))
			{
				LOG_WARN("Home screen {:s} already exists", view->getName());
				return;
			}

			bool removed = false;
			if (inVector(s_openScreens, view))
			{
				removed = removeFromVector(s_openScreens, view);
			}
			s_homeScreens.push_back(view);

			if (removed && s_openScreens.empty() && s_openModals.empty())
			{
				/* Home screen was previously visible so it should remain visible */
				home();
			}
			notifySideBar();
		}

		void removeHomeScreenImpl(LvObjPtr view, bool close)
		{
			ZoneScoped;
			UI_LOCK();
			if (removeFromVector(s_homeScreens, view))
			{
				if (s_openScreens.empty())
				{
					/* Home screen was previously visible so it should remain visible */
					openScreenImpl(view, false);
				}
				else if (close)
				{
					closeScreenImpl(view, false);
				}
				notifySideBar();
			}
		}

		/**
		 * @brief Opens a specified screen and optionally closes the previous screen.
		 *
		 * This function attempts to open the screen specified by the `view` parameter.
		 * If `closePrevious` is true, the last opened screen will be closed before opening the new one.
		 *
		 * @param view The screen to be opened. Must not be nullptr.
		 * @param closePrevious If true, the last opened screen will be closed before opening the new one.
		 *
		 * @note If `view` is nullptr, a warning will be logged and the function will return without opening any screen.
		 * @note If the screen is already visible, it will not be shown again.
		 * @note The screen will be removed from the list of returnable screens and added to the list of open screens if
		 * it is not a home screen.
		 */
		void openScreenImpl(LvObjPtr view, bool closePrevious)
		{
			ZoneScoped;
			UI_LOCK();
			if (view == nullptr)
			{
				LOG_WARN("Trying to open a nullptr screen");
				return;
			}

			if (closePrevious && !s_openScreens.empty() && s_openScreens.back() == view && view->isVisible())
			{
				LOG_DBG("Screen '{:s}' is already the current screen", view->getName());
				closeAllModals();
				notifySideBar();
				return;
			}

			closeAllModals();

			if (closePrevious)
			{
				closeLastScreen();
			}

			LOG_INFO("Opening screen '{:s}'", view->getName());
			removeFromVector(s_returnableScreens, view);
			if (!inVector(s_homeScreens, view))
			{
				for (auto& home : s_homeScreens)
				{
					if (home->isVisible())
					{
						home->hide();
					}
				}
				addToVector(s_openScreens, view);
			}
			view->show(true);
			Transitions::fadeIn(*view);
			notifySideBar();
		}

		/**
		 * @brief Closes the specified screen and updates screen vectors accordingly.
		 *
		 * This function hides the given screen if it is currently visible, removes it from the list of open screens,
		 * and optionally adds it to the list of returnable screens.
		 *
		 * @param view The screen to be closed.
		 * @param returnable If true, the screen will be added to the list of returnable screens.
		 */
		bool closeScreenImpl(LvObjPtr view, bool returnable)
		{
			ZoneScoped;
			UI_LOCK();
			LOG_INFO("Closing screen '{:s}'", view->getName());
			if (view == nullptr)
			{
				LOG_WARN("Trying to close a nullptr screen");
				return false;
			}

			closeAllModals();

			if (view->isVisible())
			{
				view->hide();
			}
			bool removed = removeFromVector(s_openScreens, view);
			if (returnable)
			{
				addToVector(s_returnableScreens, view);
			}

			if (s_openScreens.empty() && removed)
			{
				for (auto& home : s_homeScreens)
				{
					home->show();
					Transitions::fadeIn(*home);
				}
			}
			notifySideBar();
			return removed;
		}

		void openModalImpl(LvObj* view)
		{
			ZoneScoped;
			UI_LOCK();
			if (view == nullptr)
			{
				LOG_WARN("Trying to open a nullptr modal screen");
				return;
			}

			LOG_INFO("Opening modal '{:s}'", view->getName());

			addToVector(s_openModals, view);
			view->show(true);
			notifySideBar();
		}

		bool closeModalImpl(LvObj* view)
		{
			ZoneScoped;
			UI_LOCK();
			if (view == nullptr)
			{
				LOG_WARN("Trying to close a nullptr modal");
				return false;
			}

			if (s_openModals.empty())
			{
				LOG_DBG("No open modals");
				return false;
			}

			if (!removeFromVector(s_openModals, view))
			{
				LOG_WARN("Modal '{:s}' not found in open modals", view->getName());
				return false;
			}

			LOG_INFO("Closing modal '{:s}'", view->getName());
			view->hide();
			notifySideBar();
			return true;
		}
	} // namespace detail

	void closeAllModals()
	{
		ZoneScoped;
		UI_LOCK();
		LOG_INFO("Closing all modals");
		for (auto modal : s_openModals)
		{
			LOG_INFO("Closing modal '{:s}'", modal->getName());
			modal->hide();
		}
		s_openModals.clear();
		notifySideBar();
	}

	bool closeLastModal()
	{
		ZoneScoped;
		UI_LOCK();
		if (s_openModals.empty())
		{
			LOG_DBG("No open modals to close");
			return false;
		}

		LOG_DBG("Closing last modal");
		LvObj* lastModal = s_openModals.back();
		while (!lastModal->isVisible())
		{
			LOG_DBG("Last modal '{:s}' is not visible, removing from list", lastModal->getName());
			s_openModals.pop_back();
			if (s_openModals.empty())
			{
				LOG_DBG("No more modals to close");
				return false;
			}
			lastModal = s_openModals.back();
		}
		LOG_INFO("Closing modal '{:s}'", lastModal->getName());
		lastModal->hide();
		s_openModals.pop_back();
		notifySideBar();
		return true;
	}
} // namespace UI
