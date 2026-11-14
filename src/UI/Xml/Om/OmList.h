/*
 * OmList.h
 *
 * An object model array as a list XML can show with `<ds_list>`:
 *
 *     <ds_list list="om.heat.heaters" item="heater_row"/>
 *
 * There is a row per item, nulls included, so a row's `index` is the item's index in the array: the heater, fan or
 * tool number. Each row's scope gives a name for every field of its item, relative to it (`current`, `model.pid.p`,
 * `active[0]`, `heaters`, a list), so the item component reads like the object model. A row also has `value` (an item
 * that isn't an object, as text; `value|int` etc. converts it) and `exists` (0 for a null item, so XML can hide it).
 */

#pragma once

#include "ObjectModel/Tree/Path.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Resolver.h"
#include "UI/Xml/Binding/Subjects.h"
#include "OmField.h"

namespace OM
{
	class Node;
}

namespace UI::Xml
{
	class OmBindings;

	/** One item of an object model array */
	class OmRow : public Row, public Resolver
	{
	  public:
		/** Declare `value` and `exists`, and that the other names come from the item */
		void fields(FieldVisitor& f) override;

		/**
		 * Show the item at `path`
		 * @param bindings Where the names relative to the item come from
		 * @param node The item (the model lock held)
		 */
		void show(OmBindings& bindings, const OM::Path& path, const OM::Node& node);

		/** A field of the item: its own, or with `last`, any name that could be one (see Resolver.h) */
		Subject* findSubject(const lv_xml_subject_ctx_t* origin, std::string_view name, bool last) override;

		/** An array of the item: as findSubject() */
		ListBase* findList(const lv_xml_subject_ctx_t* origin, std::string_view name, bool last) override;

		StringSubject<OmField::TEXT_SIZE> value; // The item as text, if it isn't an object
		IntSubject exists;						 // 1 unless the item is null

	  private:
		/**
		 * The path a name relative to the item stands for, or nothing if it isn't one
		 * @param relative The name's path, e.g. "model.pid.p"
		 * @param last Whether any field will do; otherwise the item must have the field
		 */
		std::optional<OM::Path> resolve(std::string_view relative, bool last) const;

		OmBindings* m_bindings = nullptr; // Where names come from; nullptr until shown
		OM::Path m_path;				  // The item
	};

	/** An object model array */
	class OmList : public List<OmRow>
	{
	  public:
		/**
		 * @param bindings Where the rows' names come from
		 * @param path The array
		 */
		OmList(OmBindings& bindings, OM::Path path);

		/** The array */
		const OM::Path& path() const { return m_path; }

		/** Read the array from the object model `root` again (the model lock held) */
		void refresh(const OM::Node& root);

	  private:
		OmBindings& m_bindings; // Where the rows' names come from
		OM::Path m_path;		// The array
	};
} // namespace UI::Xml
