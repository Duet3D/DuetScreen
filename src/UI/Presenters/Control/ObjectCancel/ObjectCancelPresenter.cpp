/*
 * ObjectCancelPresenter.cpp
 */

#include "ObjectCancelPresenter.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
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
		f.field("name", name, "Its name, as the slicer wrote it, e.g. \"gear (Instance 37)\"");
		f.field("x1", x1, "Lowest X of its bounds, whole mm");
		f.field("y1", y1, "Lowest Y of its bounds, whole mm");
		f.field("x2", x2, "Highest X of its bounds, whole mm");
		f.field("y2", y2, "Highest Y of its bounds, whole mm");
		f.field("current", current, "1 if it is being printed");
		f.field("cancelled", cancelled, "1 if it has been cancelled");
		f.field("chosen", chosen, "1 if it is the one chosen to cancel or resume");
	}

	ObjectCancelPresenter::ObjectCancelPresenter()
		: XmlPresenter("object_cancel")
	{
		action("object_cancel.choose",
			   "Choose the object to cancel or resume. From a row of `objects`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto* row = args.row<JobObjectRow>();
				   if (row == nullptr)
				   {
					   LOG_WARN("object_cancel.choose must come from an objects row");
					   return;
				   }
				   m_userChoice = row->number.get();
				   showObjects();
			   });
		action("object_cancel.cancel",
			   "Cancel the chosen object (M486 P), asking first if confirmations are on",
			   [this](const Xml::ActionArgs& args) { cancelChosen(false, args.target); });
		action("object_cancel.resume",
			   "Print the chosen object again after it was cancelled (M486 U), asking first if confirmations are on",
			   [this](const Xml::ActionArgs& args) { cancelChosen(true, args.target); });
		m_currentObject.set(_("object_cancel.current_object_none"));
	}

	void ObjectCancelPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("current_object", m_currentObject, "Name of the object being printed, or \"Nothing\"");
		f.field("chosen.name",
				m_chosenName,
				"Name of the object chosen: the one the user tapped, or else the one being printed");
		f.field("chosen.cancelled", m_chosenCancelled, "1 if the object chosen is cancelled");
		f.field("chosen.shown", m_chosenShown, "1 if an object is chosen");
		f.field("chosen.cancel_text", m_cancelText, "E.g. \"Cancel gear (Instance 37)…\"");
		f.field("chosen.resume_text", m_resumeText, "E.g. \"Resume gear (Instance 37)…\"");
		f.field("x_min",
				m_xMin,
				"Lowest X of the bed, whole mm: the X axis' travel allowing for the tool's offset, or the objects' "
				"bounds with a margin if the axes aren't known");
		f.field("x_max", m_xMax, "Highest X of the bed, whole mm; as `x_min`");
		f.field("y_min", m_yMin, "Lowest Y of the bed, whole mm; as `x_min`");
		f.field("y_max", m_yMax, "Highest Y of the bed, whole mm; as `x_min`");
		f.list("objects", m_objects, "The job's objects");
	}

	void ObjectCancelPresenter::onInit()
	{
		m_watch = Xml::OmWatch({"job.build", "move.axes", "tools", "state.currentTool"}, [this] { showObjects(); });
	}

	void ObjectCancelPresenter::onActivate()
	{
		showObjects();
	}

	const OM::Node& ObjectCancelPresenter::objects()
	{
		return Xml::PrinterModel::root()["job"]["build"]["objects"];
	}

	std::string ObjectCancelPresenter::currentObjectName()
	{
		const int64_t current = Xml::PrinterModel::root()["job"]["build"]["currentObject"].integer(-1);
		const std::string_view name = current >= 0 ? objects()[static_cast<size_t>(current)]["name"].text() : "";
		return name.empty() ? _("object_cancel.current_object_none") : std::string(name);
	}

	int32_t ObjectCancelPresenter::chosenObject() const
	{
		if (m_userChoice >= 0 && objects()[static_cast<size_t>(m_userChoice)].exists())
		{
			return m_userChoice;
		}
		const int64_t current = Xml::PrinterModel::root()["job"]["build"]["currentObject"].integer(-1);
		return current >= 0 && objects()[static_cast<size_t>(current)].exists() ? static_cast<int32_t>(current) : -1;
	}

	void ObjectCancelPresenter::cancelChosen(bool resume, lv_obj_t* asker)
	{
		int32_t chosen = -1;
		std::string name;
		{
			MODEL_LOCK();
			chosen = chosenObject();
			if (chosen < 0)
			{
				return;
			}
			name = objects()[static_cast<size_t>(chosen)]["name"].text();
		}
		// M486 P cancels an object, M486 U resumes it
		const std::string gcode = fmt::format("M486 {:c}{:d}\n", resume ? 'U' : 'P', chosen);
		Xml::Prompts::confirm(
			{.title = _(resume ? "object_cancel.confirm_resume_title" : "object_cancel.confirm_cancel_title"),
			 .text = _(resume ? "object_cancel.confirm_resume_text" : "object_cancel.confirm_cancel_text", name),
			 .yes = _(resume ? "object_cancel.resume" : "object_cancel.cancel"),
			 .onYes = [gcode]() { Xml::Services::sendGcode(gcode); },
			 .asker = asker});
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
		// A choice is forgotten once its object has gone, e.g. with the job
		if (m_userChoice >= 0 && !objects()[static_cast<size_t>(m_userChoice)].exists())
		{
			m_userChoice = -1;
		}
		const int64_t current = Xml::PrinterModel::root()["job"]["build"]["currentObject"].integer(-1);
		const int32_t chosen = chosenObject();
		m_objects.sync(numbers.size(),
					   [&numbers, current, chosen](JobObjectRow& row, size_t i)
					   {
						   const OM::Node& object = objects()[numbers[i]];
						   row.number.set(static_cast<int32_t>(numbers[i]));
						   row.name.set(object["name"].text());
						   row.x1.set(static_cast<int32_t>(object["x"][0].integer(0)));
						   row.y1.set(static_cast<int32_t>(object["y"][0].integer(0)));
						   row.x2.set(static_cast<int32_t>(object["x"][1].integer(0)));
						   row.y2.set(static_cast<int32_t>(object["y"][1].integer(0)));
						   row.current.set(static_cast<int64_t>(numbers[i]) == current ? 1 : 0);
						   row.cancelled.set(object["cancelled"].boolean() ? 1 : 0);
						   row.chosen.set(static_cast<int32_t>(numbers[i]) == chosen ? 1 : 0);
					   });
		m_currentObject.set(currentObjectName());

		const std::string name = chosen >= 0 ? std::string(objects()[static_cast<size_t>(chosen)]["name"].text()) : "";
		m_chosenName.set(name);
		m_chosenShown.set(chosen >= 0 ? 1 : 0);
		m_chosenCancelled.set(chosen >= 0 && objects()[static_cast<size_t>(chosen)]["cancelled"].boolean() ? 1 : 0);
		m_cancelText.set(_("object_cancel.cancel_named", name));
		m_resumeText.set(_("object_cancel.resume_named", name));
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
