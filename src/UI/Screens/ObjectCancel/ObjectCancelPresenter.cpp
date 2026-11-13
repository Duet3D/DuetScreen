/*
 * ObjectCancelPresenter.cpp
 */

#include "ObjectCancelPresenter.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/Tool.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace UI
{
	/** The span shown when neither the axes nor any objects are known, mm */
	static constexpr float DEFAULT_SPAN = 300.0f;

	/** The margin around the objects when the axes aren't known, as a fraction of their span */
	static constexpr float OBJECT_MARGIN = 0.1f;

	void JobObjectRow::fields(Xml::FieldVisitor& f)
	{
		f.field("number", number, "Object number");
		f.field("label", label, "E.g. \"[2]: gear (Instance 37)\"");
		f.field("x1", x1, "Lowest X of its bounds, whole mm");
		f.field("y1", y1, "Lowest Y of its bounds, whole mm");
		f.field("x2", x2, "Highest X of its bounds, whole mm");
		f.field("y2", y2, "Highest Y of its bounds, whole mm");
		f.field("current", current, "1 if it is being printed");
		f.field("cancelled", cancelled, "1 if it has been cancelled");
	}

	ObjectCancelPresenter::ObjectCancelPresenter()
		: XmlPresenter("object_cancel")
	{
		action("object_cancel.select",
			   "Cancel the object, or resume it if cancelled, asking first. From a row of `objects`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto* row = args.row<JobObjectRow>();
				   auto object = row ? OM::GetJobObject(static_cast<size_t>(row->number.get())) : nullptr;
				   if (object == nullptr)
				   {
					   LOG_WARN("object_cancel.select must come from an objects row");
					   return;
				   }
				   const size_t index = object->index;
				   const bool resume = object->cancelled;
				   if (resume)
				   {
					   ask(_("object_cancel.confirm_uncancel_title"),
						   _("object_cancel.confirm_uncancel_text", object->name),
						   [index]() { OM::SetJobObjectActive(index, true); });
				   }
				   else
				   {
					   ask(_("object_cancel.confirm_cancel_title"),
						   _("object_cancel.confirm_cancel_text", object->name),
						   [index]() { OM::SetJobObjectActive(index, false); });
				   }
			   });
		action("object_cancel.cancel_current",
			   "Cancel the object being printed, asking first if confirmations are on",
			   [this](const Xml::ActionArgs&)
			   {
				   if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
				   {
					   OM::CancelCurrentJobObject();
					   return;
				   }
				   const int8_t current = OM::GetCurrentJobObjectIndex();
				   auto object = current >= 0 ? OM::GetJobObject(static_cast<size_t>(current)) : nullptr;
				   const std::string name = object != nullptr && !object->name.empty()
												? std::string(object->name)
												: _("object_cancel.current_object_none");
				   ask(_("object_cancel.confirm_cancel_current_title"),
					   _("object_cancel.confirm_cancel_current_text", name),
					   []() { OM::CancelCurrentJobObject(); });
			   });
		action("object_cancel.confirm",
			   "Answer yes to the question (`confirm`): close it and do what it asked",
			   [this](const Xml::ActionArgs&)
			   {
				   m_confirm.set(0);
				   if (auto action = std::move(m_confirmed))
				   {
					   action();
				   }
			   });
		action("object_cancel.close_dialog",
			   "Answer no to the question (`confirm`): close it without doing anything",
			   [this](const Xml::ActionArgs&)
			   {
				   m_confirm.set(0);
				   m_confirmed = nullptr;
			   });
		m_currentObject.set(_("object_cancel.current_object_none"));
	}

	void ObjectCancelPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("current_object", m_currentObject, "Name of the object being printed, or \"None\"");
		f.field("x_min",
				m_xMin,
				"Lowest X of the bed, whole mm: the X axis' travel allowing for the tool's offset, or the objects' "
				"bounds with a margin if the axes aren't known");
		f.field("x_max", m_xMax, "Highest X of the bed, whole mm; as `x_min`");
		f.field("y_min", m_yMin, "Lowest Y of the bed, whole mm; as `x_min`");
		f.field("y_max", m_yMax, "Highest Y of the bed, whole mm; as `x_min`");
		f.list("objects", m_objects, "The job's objects");
		f.field("confirmations", m_confirmations, "1 if actions are confirmed first (the setting)");
		f.field("confirm", m_confirm, "1 while asking; `confirm_title` and `confirm_text` are the question");
		f.field("confirm_title", m_confirmTitle, "Question title");
		f.field("confirm_text", m_confirmText, "Question");
	}

	void ObjectCancelPresenter::onInit()
	{
		registerEventListener<EventType::JobBuild>([this]() { showObjects(); });
		registerEventListener<EventType::JobCurrentObject>([this]() { showObjects(); });
		registerEventListener<EventType::JobObjectData>([this]() { showObjects(); });
		registerEventListener<EventType::AxesData>([this]() { showSpan(); });
	}

	void ObjectCancelPresenter::onActivate()
	{
		m_confirmations.set(StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS) ? 1 : 0);
		showObjects();
	}

	void ObjectCancelPresenter::ask(std::string title, std::string text, std::function<void()> action)
	{
		m_confirmTitle.set(title);
		m_confirmText.set(text);
		m_confirmed = std::move(action);
		m_confirm.set(1);
	}

	void ObjectCancelPresenter::showObjects()
	{
		ZoneScoped;
		MODEL_LOCK();
		showSpan();

		std::vector<std::shared_ptr<OM::JobObject>> objects;
		objects.reserve(OM::GetJobObjectCount());
		OM::IterateJobObjectsWhile(
			[&objects](std::shared_ptr<OM::JobObject> object, size_t)
			{
				objects.push_back(std::move(object));
				return true;
			});
		const int8_t current = OM::GetCurrentJobObjectIndex();
		m_objects.sync(objects.size(),
					   [&objects, current](JobObjectRow& row, size_t i)
					   {
						   const auto& object = *objects[i];
						   row.number.set(static_cast<int32_t>(object.index));
						   row.label.set(fmt::format("[{:d}]: {:s}", object.index, object.name));
						   row.x1.set(object.bounds.x[0]);
						   row.y1.set(object.bounds.y[0]);
						   row.x2.set(object.bounds.x[1]);
						   row.y2.set(object.bounds.y[1]);
						   row.current.set(static_cast<int8_t>(object.index) == current ? 1 : 0);
						   row.cancelled.set(object.cancelled ? 1 : 0);
					   });

		auto currentObject = current >= 0 ? OM::GetJobObject(static_cast<size_t>(current)) : nullptr;
		m_currentObject.set(currentObject != nullptr && !currentObject->name.empty()
								? std::string(currentObject->name)
								: _("object_cancel.current_object_none"));
	}

	void ObjectCancelPresenter::showSpan()
	{
		MODEL_LOCK();
		auto axisX = OM::Move::GetAxisByLetter('X');
		auto axisY = OM::Move::GetAxisByLetter('Y');
		float xMin = 0.0f;
		float xMax = DEFAULT_SPAN;
		float yMin = 0.0f;
		float yMax = DEFAULT_SPAN;
		if (axisX)
		{
			const float offset = OM::GetCurrentToolAxisOffset(axisX);
			xMin = axisX->minPosition + offset;
			xMax = axisX->maxPosition + offset;
		}
		if (axisY)
		{
			const float offset = OM::GetCurrentToolAxisOffset(axisY);
			yMin = axisY->minPosition + offset;
			yMax = axisY->maxPosition + offset;
		}

		// Without the axes, the objects' bounds with a margin
		if (!axisX || !axisY)
		{
			bool any = false;
			float oxMin = 0.0f;
			float oxMax = 0.0f;
			float oyMin = 0.0f;
			float oyMax = 0.0f;
			OM::IterateJobObjectsWhile(
				[&](std::shared_ptr<OM::JobObject> object, size_t)
				{
					const auto& b = object->bounds;
					oxMin = any ? std::min(oxMin, static_cast<float>(b.x[0])) : static_cast<float>(b.x[0]);
					oxMax = any ? std::max(oxMax, static_cast<float>(b.x[1])) : static_cast<float>(b.x[1]);
					oyMin = any ? std::min(oyMin, static_cast<float>(b.y[0])) : static_cast<float>(b.y[0]);
					oyMax = any ? std::max(oyMax, static_cast<float>(b.y[1])) : static_cast<float>(b.y[1]);
					any = true;
					return true;
				});
			if (!any)
			{
				xMin = yMin = 0.0f;
				xMax = yMax = DEFAULT_SPAN;
			}
			else
			{
				const float marginX = (oxMax - oxMin) * OBJECT_MARGIN;
				const float marginY = (oyMax - oyMin) * OBJECT_MARGIN;
				if (!axisX)
				{
					xMin = oxMin - marginX;
					xMax = oxMax + marginX;
				}
				if (!axisY)
				{
					yMin = oyMin - marginY;
					yMax = oyMax + marginY;
				}
			}
		}
		m_xMin.set(static_cast<int32_t>(std::floor(xMin)));
		m_xMax.set(static_cast<int32_t>(std::ceil(xMax)));
		m_yMin.set(static_cast<int32_t>(std::floor(yMin)));
		m_yMax.set(static_cast<int32_t>(std::ceil(yMax)));
	}
} // namespace UI
