/*
 * ObjectCancelPresenter.cpp
 *
 *  Created on: 2025-03-18
 *      Author: Andy Everitt
 */

#include "ObjectCancelPresenter.h"
#include "Debug.h"
#include "ObjectCancelView.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/Tool.h"
#include "tracy/Tracy.hpp"
#include <algorithm>
#include <cmath>

namespace UI
{
	namespace
	{
		float getToolOffsetForAxis(const OM::Move::AxisPtr& axis)
		{
			if (axis == nullptr)
			{
				return 0.0f;
			}

			const char axisLetter = axis->letter[0];
			if (axisLetter != 'X' && axisLetter != 'Y')
			{
				return 0.0f;
			}

			auto tool = OM::GetCurrentTool();
			if (tool == nullptr || axis->index >= MAX_TOTAL_AXES)
			{
				return 0.0f;
			}

			return tool->offsets[axis->index];
		}
	} // namespace

	void ObjectCancelPresenter::onActivate()
	{
		ZoneScoped;
		render();
	}

	void ObjectCancelPresenter::render()
	{
		ZoneScoped;
		auto* view = getView();

		updateAxisRange();
		view->beginObjectOverlayUpdate();

		const size_t count = OM::GetJobObjectCount();
		view->setObjectCount(count);

		const int8_t currentIdx = OM::GetCurrentJobObjectIndex();
		std::string currentObjectName;
		if (currentIdx >= 0)
		{
			auto currentObj = OM::GetJobObject(static_cast<size_t>(currentIdx));
			if (currentObj != nullptr)
			{
				currentObjectName = currentObj->name;
			}
		}
		view->setCurrentObjectName(currentObjectName);

		OM::IterateJobObjectsWhile(
			[view, currentIdx](std::shared_ptr<OM::JobObject> obj, size_t /*i*/)
			{
				const bool isCurrent = (static_cast<int8_t>(obj->index) == currentIdx);
				view->updateObjectOverlay(obj->index, obj->bounds, isCurrent, obj->cancelled);
				view->updateObjectItem(obj->index, obj->name, isCurrent, obj->cancelled);
				return true;
			});
	}

	void ObjectCancelPresenter::updateAxisRange()
	{
		ZoneScoped;

		auto* view = getView();

		auto axisX = OM::Move::GetAxisByLetter('X');
		auto axisY = OM::Move::GetAxisByLetter('Y');

		float xMin = axisX ? axisX->minPosition : 0.0f;
		float xMax = axisX ? axisX->maxPosition : 300.0f;
		float yMin = axisY ? axisY->minPosition : 0.0f;
		float yMax = axisY ? axisY->maxPosition : 300.0f;

		if (axisX)
		{
			const float xOffset = getToolOffsetForAxis(axisX);
			xMin += xOffset;
			xMax += xOffset;
		}
		if (axisY)
		{
			const float yOffset = getToolOffsetForAxis(axisY);
			yMin += yOffset;
			yMax += yOffset;
		}

		// If no axis data, derive range from object bounds (with small margin)
		if (!axisX || !axisY)
		{
			bool hasObjects = false;
			OM::IterateJobObjectsWhile(
				[&](std::shared_ptr<OM::JobObject> obj, size_t)
				{
					if (!axisX)
					{
						if (!hasObjects)
						{
							xMin = static_cast<float>(obj->bounds.x[0]);
							xMax = static_cast<float>(obj->bounds.x[1]);
						}
						else
						{
							xMin = std::min(xMin, static_cast<float>(obj->bounds.x[0]));
							xMax = std::max(xMax, static_cast<float>(obj->bounds.x[1]));
						}
					}
					if (!axisY)
					{
						if (!hasObjects)
						{
							yMin = static_cast<float>(obj->bounds.y[0]);
							yMax = static_cast<float>(obj->bounds.y[1]);
						}
						else
						{
							yMin = std::min(yMin, static_cast<float>(obj->bounds.y[0]));
							yMax = std::max(yMax, static_cast<float>(obj->bounds.y[1]));
						}
					}
					hasObjects = true;
					return true;
				});

			if (!hasObjects)
			{
				xMin = 0.0f;
				xMax = 300.0f;
				yMin = 0.0f;
				yMax = 300.0f;
			}
			else
			{
				// Add 10% margin
				const float marginX = (xMax - xMin) * 0.1f;
				const float marginY = (yMax - yMin) * 0.1f;
				xMin -= marginX;
				xMax += marginX;
				yMin -= marginY;
				yMax += marginY;
			}
		}

		view->setAxisRange({static_cast<int32_t>(std::floor(xMin)), static_cast<int32_t>(std::ceil(xMax))},
						   {static_cast<int32_t>(std::floor(yMin)), static_cast<int32_t>(std::ceil(yMax))});
	}

	void ObjectCancelPresenter::setJobObjectActive(size_t index, bool active)
	{
		ZoneScoped;
		LOG_INFO("ObjectCancel: setting object {:d} active={}", index, active);
		OM::SetJobObjectActive(index, active);
	}

	void ObjectCancelPresenter::cancelCurrentObject()
	{
		ZoneScoped;
		LOG_INFO("ObjectCancel: cancelling current object");
		OM::CancelCurrentJobObject();
	}

	void ObjectCancelPresenter::onObjectSelected(size_t index)
	{
		ZoneScoped;
		auto obj = OM::GetJobObject(index);
		if (obj == nullptr)
		{
			LOG_WARN("ObjectCancel: selected object {:d} not found", index);
			return;
		}
		getView()->showConfirmModal(obj->index, obj->name, obj->cancelled);
	}

	void ObjectCancelPresenter::newJobBuild()
	{
		ZoneScoped;
		LOG_INFO("ObjectCancel: new job build");
		render();
	}

	void ObjectCancelPresenter::newJobCurrentObject()
	{
		ZoneScoped;
		render();
	}

	void ObjectCancelPresenter::newJobObjectData()
	{
		ZoneScoped;
		render();
	}

	void ObjectCancelPresenter::newAxesData()
	{
		ZoneScoped;
		if (isActive())
		{
			updateAxisRange();
		}
	}
} // namespace UI
