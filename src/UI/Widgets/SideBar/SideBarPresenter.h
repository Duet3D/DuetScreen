/*
 * SideBarPresenter.h
 *
 *  Created on: 2025-02-21
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Core/Presenter.h"

namespace UI
{
	class SideBar;

	class SideBarPresenter : public Presenter<SideBar>
	{
	  public:
		PRESENTER_CONSTRUCTOR(SideBarPresenter, SideBar)

	  protected:
		void enableBackButton(bool enable);
		void enableHomeButton(bool enable);
		void setSelectedView(LvObj* view);
		void newResponse(const ResponseType type, const std::string& message);

		void onInit() override;
		void onActivate() override;
	};
} // namespace UI
