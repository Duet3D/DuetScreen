/*
 * AppDrawer.h
 *
 *  Created on: 2025-08-05
 *      Author: Andy Everitt
 */

#pragma once

#include "App.h"
#include "UI/Components/Containers/Card.h"
#include "UI/Components/List/List.h"

namespace UI
{
	class AppDrawer : public Card
	{
	  public:
		AppDrawer(const std::string& name, LvObj& parent);
		void init();

	  private:
		List<App> m_apps; // List of apps in the drawer
	};
} // namespace UI
