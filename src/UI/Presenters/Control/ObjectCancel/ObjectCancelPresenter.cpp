/*
 * ObjectCancelPresenter.cpp
 */

#include "ObjectCancelPresenter.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/Services.h"
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
			   [](const Xml::ActionArgs& args)
			   {
				   const auto* row = args.row<JobObjectRow>();
				   if (row == nullptr)
				   {
					   LOG_WARN("object_cancel.select must come from an objects row");
					   return;
				   }
				   const auto index = static_cast<size_t>(row->number.get());
				   std::string name;
				   bool resume = false;
				   {
					   MODEL_LOCK();
					   const OM::Node& object = objects()[index];
					   if (!object.exists())
					   {
						   return;
					   }
					   name = object["name"].text();
					   resume = object["cancelled"].boolean();
				   }
				   // M486 P cancels an object, M486 U resumes it
				   const std::string gcode = fmt::format("M486 {:c}{:d}\n", resume ? 'U' : 'P', index);
				   if (resume)
				   {
					   ask(args.target,
						   _("object_cancel.confirm_uncancel_title"),
						   _("object_cancel.confirm_uncancel_text", name),
						   [gcode]() { Xml::Services::sendGcode(gcode); });
				   }
				   else
				   {
					   ask(args.target,
						   _("object_cancel.confirm_cancel_title"),
						   _("object_cancel.confirm_cancel_text", name),
						   [gcode]() { Xml::Services::sendGcode(gcode); });
				   }
			   });
		action("object_cancel.cancel_current",
			   "Cancel the object being printed, asking first if confirmations are on",
			   [](const Xml::ActionArgs& args)
			   {
				   const auto cancelCurrent = []() { Xml::Services::sendGcode("M486 C\n"); };
				   if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
				   {
					   cancelCurrent();
					   return;
				   }
				   ask(args.target,
					   _("object_cancel.confirm_cancel_current_title"),
					   _("object_cancel.confirm_cancel_current_text", currentObjectName()),
					   cancelCurrent);
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
	}

	void ObjectCancelPresenter::onInit()
	{
		m_watch = Xml::OmWatch({"job.build", "move.axes", "tools", "state.currentTool"}, [this] { showObjects(); });
	}

	void ObjectCancelPresenter::onActivate()
	{
		m_confirmations.set(StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS) ? 1 : 0);
		showObjects();
	}

	void ObjectCancelPresenter::ask(lv_obj_t* asker, std::string title, std::string text, std::function<void()> action)
	{
		Xml::Prompts::askQuestion(
			{.title = std::move(title), .text = std::move(text), .onYes = std::move(action), .asker = asker});
	}

	const OM::Node& ObjectCancelPresenter::objects()
	{
		return Xml::PrinterModel::root()["job"]["build"]["objects"];
	}

	std::string ObjectCancelPresenter::currentObjectName()
	{
		MODEL_LOCK();
		const int64_t current = Xml::PrinterModel::root()["job"]["build"]["currentObject"].integer(-1);
		const std::string_view name = current >= 0 ? objects()[static_cast<size_t>(current)]["name"].text() : "";
		return name.empty() ? _("object_cancel.current_object_none") : std::string(name);
	}

	void ObjectCancelPresenter::showObjects()
	{
		ZoneScoped;
		MODEL_LOCK();
		showSpan();

		std::vector<size_t> numbers;
		for (size_t number = 0; number < objects().size(); ++number)
		{
			if (objects()[number].exists())
			{
				numbers.push_back(number);
			}
		}
		const int64_t current = Xml::PrinterModel::root()["job"]["build"]["currentObject"].integer(-1);
		m_objects.sync(numbers.size(),
					   [&numbers, current](JobObjectRow& row, size_t i)
					   {
						   const OM::Node& object = objects()[numbers[i]];
						   row.number.set(static_cast<int32_t>(numbers[i]));
						   row.label.set(fmt::format("[{:d}]: {:s}", numbers[i], object["name"].text()));
						   row.x1.set(static_cast<int32_t>(object["x"][0].integer(0)));
						   row.y1.set(static_cast<int32_t>(object["y"][0].integer(0)));
						   row.x2.set(static_cast<int32_t>(object["x"][1].integer(0)));
						   row.y2.set(static_cast<int32_t>(object["y"][1].integer(0)));
						   row.current.set(static_cast<int64_t>(numbers[i]) == current ? 1 : 0);
						   row.cancelled.set(object["cancelled"].boolean() ? 1 : 0);
					   });
		m_currentObject.set(currentObjectName());
	}

	void ObjectCancelPresenter::showSpan()
	{
		MODEL_LOCK();
		const OM::Node& axisX = Xml::PrinterModel::axis('X');
		const OM::Node& axisY = Xml::PrinterModel::axis('Y');
		float xMin = 0.0f;
		float xMax = DEFAULT_SPAN;
		float yMin = 0.0f;
		float yMax = DEFAULT_SPAN;
		if (axisX.exists())
		{
			const float offset = Xml::PrinterModel::toolOffset('X');
			xMin = static_cast<float>(axisX["min"].number()) + offset;
			xMax = static_cast<float>(axisX["max"].number()) + offset;
		}
		if (axisY.exists())
		{
			const float offset = Xml::PrinterModel::toolOffset('Y');
			yMin = static_cast<float>(axisY["min"].number()) + offset;
			yMax = static_cast<float>(axisY["max"].number()) + offset;
		}

		// Without the axes, the objects' bounds with a margin
		if (!axisX.exists() || !axisY.exists())
		{
			bool any = false;
			float oxMin = 0.0f;
			float oxMax = 0.0f;
			float oyMin = 0.0f;
			float oyMax = 0.0f;
			for (size_t number = 0; number < objects().size(); ++number)
			{
				const OM::Node& object = objects()[number];
				if (!object.exists())
				{
					continue;
				}
				const auto x0 = static_cast<float>(object["x"][0].number());
				const auto x1 = static_cast<float>(object["x"][1].number());
				const auto y0 = static_cast<float>(object["y"][0].number());
				const auto y1 = static_cast<float>(object["y"][1].number());
				oxMin = any ? std::min(oxMin, x0) : x0;
				oxMax = any ? std::max(oxMax, x1) : x1;
				oyMin = any ? std::min(oyMin, y0) : y0;
				oyMax = any ? std::max(oyMax, y1) : y1;
				any = true;
			}
			if (!any)
			{
				xMin = yMin = 0.0f;
				xMax = yMax = DEFAULT_SPAN;
			}
			else
			{
				const float marginX = (oxMax - oxMin) * OBJECT_MARGIN;
				const float marginY = (oyMax - oyMin) * OBJECT_MARGIN;
				if (!axisX.exists())
				{
					xMin = oxMin - marginX;
					xMax = oxMax + marginX;
				}
				if (!axisY.exists())
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
