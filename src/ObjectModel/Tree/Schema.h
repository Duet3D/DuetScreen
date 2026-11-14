/*
 * Schema.h
 *
 * The object model's shape: every field the firmware can send, with its type, generated from Duet Web Control's
 * typed object model (scripts/generate_om_schema.py writes assets/object_model/schema.json). The tree (Tree.h) holds
 * what the printer has sent; the schema says what it can send, so the UI can tell an item's fields from other names
 * before the printer has sent anything, and the API lint can check `om.*` names.
 *
 * The schema only adds knowledge: a field it doesn't list (one a newer firmware added) still arrives in the tree and
 * can be shown.
 *
 * Types, as the schema file writes them: number, string, boolean, any (whatever the printer sends), an object type or
 * enum by name, `<type>[]` for an array, `{<type>}` for an object whose field names are data.
 */

#pragma once

#include "Path.h"
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace OM
{
	class Schema
	{
	  public:
		/** The schema the UI uses; empty (knowing nothing) until load() */
		static Schema& get();

		/**
		 * Read a schema file, replacing what this held
		 * @return false (with a warning) if it can't be read; the schema is then empty
		 */
		bool load(const std::filesystem::path& file);

		/** Whether nothing is known: no schema was read */
		bool empty() const { return m_types.empty(); }

		/** Where the schema came from, e.g. "@duet3d/objectmodel 3.6.4" */
		const std::string& source() const { return m_source; }

		/**
		 * The type of what is at `path`, whatever its indices (any item of an array has the same type)
		 * @return the type, or nothing if the schema doesn't know the path
		 */
		std::optional<std::string> typeAt(const Path& path) const;

		/** The values of enum `type`, or nullptr if it isn't an enum */
		const std::vector<std::string>* enumValues(std::string_view type) const;

		/**
		 * Visit every field, depth first, in name order: `path` as RRF writes it with `[]` for any item, e.g.
		 * `heat.heaters[].current`, and `{}` for any field of an object whose field names are data
		 */
		void forEachField(const std::function<void(const std::string& path, const std::string& type)>& visit) const;

	  private:
		/** Visit the fields of `type` (see forEachField()), below `prefix`, not entering types in `entered` again */
		void visitFields(const std::string& type,
						 const std::string& prefix,
						 std::vector<std::string>& entered,
						 const std::function<void(const std::string&, const std::string&)>& visit) const;

		std::string m_source;															 // Where it came from
		std::string m_root;																 // Type of the root
		std::map<std::string, std::map<std::string, std::string, std::less<>>, std::less<>> m_types; // Fields by type
		std::map<std::string, std::vector<std::string>, std::less<>> m_enums;						  // Values by enum
	};
} // namespace OM
