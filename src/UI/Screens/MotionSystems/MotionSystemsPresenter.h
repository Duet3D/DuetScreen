/*
 * MotionSystemsPresenter.h
 *
 *  Created on: 2026-03-24
 */

#pragma once

#include "UI/Core/Presenter.h"

namespace UI
{
	class MotionSystemsView;

	class MotionSystemsPresenter : public Presenter<MotionSystemsView>
	{
	  public:
		PRESENTER_CONSTRUCTOR(MotionSystemsPresenter, MotionSystemsView)

		void newMotionSystemData();
		void newAxesData();

	  protected:
		void onInit() override;
		void onActivate() override;
		void onDisconnect() override;


	  private:
		void refreshTool();
		void refreshSpeedFactor();
		void refreshSpeeds();
	};
} // namespace UI