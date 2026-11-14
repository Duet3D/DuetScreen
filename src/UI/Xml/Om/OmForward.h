/*
 * OmForward.h
 *
 * A name with a subject index (`om.heat.heaters[{row.value}].current`, see OmPath.h), or a converter argument with a
 * subject's value in it (`|default:tr.temperature.heater:{row.value}`), can stand for different fields over time,
 * but LVGL can't move a widget's binding to another subject. So the widget binds to a forwarder instead: a subject of
 * its own that observes the shared subject of the field its path points at now (an OmField), copies it, and moves to
 * another field when an index or the argument changes. Widgets keep their binding; nothing is rebuilt.
 *
 *   OmTree ──flush──▶ [heat.heaters[2].current] ──observed by──▶ [forwarder] ──▶ label
 *
 * OmForwardList does the same for a list: when its path moves, its rows are removed and the new array's are added.
 *
 * A forwarder observes its index subjects, so it must not outlive them: it is kept by the scope it was made for (see
 * Scope::keep()), whose index subjects are its own, its parents' or the globals', which all outlive it.
 */

#pragma once

#include "Converters.h"
#include "OmPath.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "OmField.h"
#include <memory>
#include <vector>

namespace UI::Xml
{
	class OmBindings;
	class OmList;

	/** A subject following a field whose path has subject indices */
	class OmForward : public Subject
	{
	  public:
		/**
		 * @param bindings Where the fields' shared subjects come from
		 * @param path The path, which may have subject indices
		 * @param converter What the subject holds
		 * @param argument The converter's argument, which may have subjects' values in it
		 */
		OmForward(OmBindings& bindings, OmBoundPath path, const Converter& converter, OmBoundText argument);

		/** Stops observing */
		~OmForward() override;

		OmForward(const OmForward&) = delete;
		OmForward& operator=(const OmForward&) = delete;

		/** The subject widgets bind to */
		lv_subject_t* raw() override { return m_own->raw(); }

		/** "int" or "text", as the converter gives */
		std::string_view typeName() const override { return m_own->typeName(); }

		/** Whether the value is `text` */
		bool equals(std::string_view text) const override { return m_own->equals(text); }

		/** The value as XML writes it */
		std::string text() const override { return m_own->text(); }

	  private:
		/** An index or the argument changed: follow the field the path and argument point at now */
		static void indexChangedCb(lv_observer_t* observer, lv_subject_t* subject);

		/** The field followed changed: copy it */
		static void targetChangedCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Follow the field the path points at now */
		void retarget();

		/** Show what the converter makes of a missing field */
		void showMissing();

		OmBindings& m_bindings;							   // Where the fields' shared subjects come from
		OmBoundPath m_path;								   // The path
		const Converter& m_converter;					   // What the subject holds
		OmBoundText m_argument;							   // The converter's argument
		std::unique_ptr<IntSubject> m_integer;			   // Own subject, for an integer converter
		std::unique_ptr<StringSubject<OmField::TEXT_SIZE>> m_textSubject; // Own subject, for a text converter
		Subject* m_own = nullptr;						   // Whichever of the two there is
		std::vector<lv_observer_t*> m_indexObservers;	   // On the index subjects
		lv_observer_t* m_targetObserver = nullptr;		   // On the field followed, or nullptr
		std::optional<OM::Path> m_target;				   // The field followed, or nothing
		std::string m_targetArgument;					   // The argument it is followed with
	};

	/** A list following an array whose path has subject indices */
	class OmForwardList : public ListBase, private ListBase::Listener
	{
	  public:
		/**
		 * @param bindings Where the arrays' lists come from
		 * @param path The array's path, with subject indices
		 */
		OmForwardList(OmBindings& bindings, OmBoundPath path);

		/** Stops observing */
		~OmForwardList() override;

		/** The rows of the array followed */
		size_t size() const override;

		/** A row of the array followed */
		Row& row(size_t index) override;

		/** The fields of a row of an object model list */
		void rowFields(FieldVisitor& f) const override;

	  private:
		/** An index changed: follow the array the path points at now */
		static void indexChangedCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Follow the array the path points at now */
		void retarget();

		/** The array followed grew or shrank: tell our listeners */
		void onListResized(size_t oldSize, size_t newSize) override;

		/** The array followed is going away: its rows go too */
		void onListDestroyed() override;

		OmBindings& m_bindings;						  // Where the arrays' lists come from
		OmBoundPath m_path;							  // The path
		std::vector<lv_observer_t*> m_indexObservers; // On the index subjects
		OmList* m_target = nullptr;					  // The array followed, or nullptr
		std::optional<OM::Path> m_targetPath;		  // Its path, or nothing
	};
} // namespace UI::Xml
