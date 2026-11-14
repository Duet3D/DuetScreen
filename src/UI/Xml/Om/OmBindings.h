/*
 * OmBindings.h
 *
 * Gives XML a name for every field of the object model (see ObjectModel/Tree/Tree.h), written as RepRapFirmware
 * writes the path, after `om.`:
 *
 *     <lv_label bind_text="om.heat.heaters[1].current"/>
 *     <lv_bar bind_value="om.fans[0].actualValue|percent"/>
 *     <ds_list list="om.tools" item="tool_card"/>
 *
 * A subject (an OmField) or list (an OmList) is made the first time a name is used, filled from the object model, and
 * kept, so every widget using the name shares it and it outlives any change to the object model. When the object
 * model changes, flush() updates what is at, below or above each changed path.
 *
 * An index may be another subject's value, `om.heat.heaters[{value}].current` (see OmPath.h): such a name gets a
 * forwarder (see OmForward.h), kept by the scope it was made for, which follows the field the path points at.
 *
 * The names are given on demand through the globals' resolver (see Binding/Resolver.h); rows of object model lists,
 * and `<ds_scope>`, give names relative to their item (see OmItemNames.h).
 */

#pragma once

#include "ObjectModel/Tree/Path.h"
#include "OmField.h"
#include "OmList.h"
#include "OmPath.h"
#include "UI/Xml/Binding/Resolver.h"
#include <functional>
#include <map>
#include <optional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	class OmBindings : public Resolver
	{
	  public:
		/**
		 * The subject for `path` read through the converter `converter` (see Converters.h), made and filled the first
		 * time it is asked for
		 * @return the subject, or nullptr (with a warning) if there is no such converter
		 */
		Subject* subject(const OM::Path& path, std::string_view converter, std::string_view argument);

		/** The list of the array at `path`, made and filled the first time it is asked for */
		OmList* list(const OM::Path& path);

		/**
		 * The subject for `path` through `converter`, for a widget built in `origin`: the shared one if `path` has no
		 * subject indices, else a forwarder kept by `origin`'s scope (or by this, if it has none)
		 * @param key The name as written, which finds the forwarder again for the same scope
		 * @return the subject, or nullptr (with a warning) if there is no such converter
		 */
		Subject* subjectFor(const OmBoundPath& path,
							std::string_view converter,
							std::string_view argument,
							const lv_xml_subject_ctx_t* origin,
							std::string_view key);

		/** As subjectFor(), for the list of an array */
		ListBase* listFor(const OmBoundPath& path, const lv_xml_subject_ctx_t* origin, std::string_view key);

		/** The bindings of the running UI (see Data/ObjectModelSource.h), or nullptr */
		static OmBindings* current();

		/** Make `bindings` the running UI's (nullptr: none) */
		static void setCurrent(OmBindings* bindings);

		/** Update everything at, below or above the paths the object model changed since the last flush, then tell
		 * the watchers of those paths */
		void flush();

		/** Read everything again, and tell every watcher */
		void refreshAll();

		/**
		 * Call `callback` after a flush changes anything at, below or above one of `paths` (e.g. "fans",
		 * "heat.heaters"), for C++ that reads the object model itself (see Data/DataSource.h)
		 * @return what unwatch() takes
		 */
		size_t watch(std::vector<std::string> paths, std::function<void()> callback);

		/** Stop the watcher watch() returned `id` for */
		void unwatch(size_t id);

		/** An `om.` name's subject (see the file comment) */
		Subject* findSubject(const lv_xml_subject_ctx_t* origin,
							 std::string_view name,
							 std::string_view written) override;

		/** An `om.` name's list (see the file comment) */
		ListBase* findList(const lv_xml_subject_ctx_t* origin,
						   std::string_view name,
						   std::string_view written) override;

	  private:
		/**
		 * The `om.` name `name`'s path, its `{name}` indices looked up from `origin`
		 * @return the path, or nothing (with a warning) if the name isn't an `om.` name that can be bound
		 */
		std::optional<OmBoundPath> bindAbsolute(const lv_xml_subject_ctx_t* origin,
												std::string_view name,
												std::string_view path) const;

		std::map<std::string, std::unique_ptr<OmField>, std::less<>> m_fields; // By path and converter, e.g. "a.b|int"
		std::map<std::string, std::unique_ptr<OmList>, std::less<>> m_lists;   // By path
		std::map<std::string, std::unique_ptr<Subject>, std::less<>> m_forwards;	// Forwarders made outside any scope
		std::map<std::string, std::unique_ptr<ListBase>, std::less<>> m_forwardLists; // As m_forwards, for lists

		/** One watch() */
		struct Watcher
		{
			size_t id;						// What unwatch() takes
			std::vector<std::string> paths; // Paths watched
			std::function<void()> callback; // Called after a flush changing one
		};
		std::vector<Watcher> m_watchers; // Told about changes
		size_t m_nextWatcher = 1;		 // Id of the next watcher
	};
} // namespace UI::Xml
