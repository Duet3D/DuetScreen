/*
 * OmPath.h
 *
 * Object model paths whose indices may be other subjects' values, so XML can join one part of the object model to
 * another:
 *
 *     om.heat.heaters[{value}].current        the heater whose number is this row's `value`
 *     om.tools[{om.state.currentTool}].name   the current tool's name
 *
 * An OmTemplate is such a path as written. Binding it (OmBoundPath::bind()) looks each `{name}` up, once, as a
 * subject; resolving the bound path (resolve()) reads those subjects to give the path they point at now.
 */

#pragma once

#include "ObjectModel/Tree/Path.h"
#include "helix_xml.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace UI::Xml
{
	/** A path as written, each index a number or `{name}` */
	class OmTemplate
	{
	  public:
		/** An index given by the subject called `name` */
		struct Ref
		{
			std::string name; // As written between the braces

			/** Refs are equal if their names are */
			bool operator==(const Ref&) const = default;
		};

		/** A field name, a number index or a subject index */
		using Segment = std::variant<std::string, size_t, Ref>;

		/**
		 * Read a path such as `heat.heaters[{value}].current`
		 * @return the template, or nothing if `text` isn't one
		 */
		static std::optional<OmTemplate> parse(std::string_view text);

		/** The segments, first first */
		const std::vector<Segment>& segments() const { return m_segments; }

		/** The first field's name (every template starts with one) */
		const std::string& firstField() const { return std::get<std::string>(m_segments.front()); }

	  private:
		std::vector<Segment> m_segments; // First first
	};

	/** An OmTemplate whose `{name}` indices have been looked up */
	class OmBoundPath
	{
	  public:
		/** Finds the subject called `name`, or nullptr */
		using Lookup = std::function<lv_subject_t*(std::string_view name)>;

		/** A path with no subject indices */
		explicit OmBoundPath(const OM::Path& path = {});

		/**
		 * Look up each `{name}` of `path` with `lookup`
		 * @param missing Set to the first name not found
		 * @return the bound path, or nothing if a name wasn't found
		 */
		static std::optional<OmBoundPath> bind(const OmTemplate& path, const Lookup& lookup, std::string& missing);

		/** This path followed by `tail` */
		OmBoundPath operator/(const OmBoundPath& tail) const;

		/** Whether an index is a subject's value, so the path can change */
		bool indirect() const;

		/** The subjects whose values are indices */
		std::vector<lv_subject_t*> subjects() const;

		/**
		 * The path the subjects point at now: an int subject's value, or a text subject's whole number
		 * @return the path, or nothing if one isn't a number of 0 or more (e.g. no current tool, -1)
		 */
		std::optional<OM::Path> resolve() const;

	  private:
		using Segment = std::variant<std::string, size_t, lv_subject_t*>;

		std::vector<Segment> m_segments; // First first
	};

	/**
	 * A converter's argument whose `{name}`s have been looked up, e.g. `tr.temperature.heater:{row.value}`: the text it
	 * stands for follows the subjects' values
	 */
	class OmBoundText
	{
	  public:
		/**
		 * Look up each `{name}` of `text` with `lookup`
		 * @param missing Set to the first name not found
		 * @return the bound text, or nothing if a name wasn't found or a brace isn't closed
		 */
		static std::optional<OmBoundText> bind(std::string_view text,
											   const OmBoundPath::Lookup& lookup,
											   std::string& missing);

		/** Whether part of it is a subject's value, so the text can change */
		bool indirect() const;

		/** The subjects whose values are part of it */
		std::vector<lv_subject_t*> subjects() const;

		/** The text now: each subject's value as text (an int subject's number) in its place */
		std::string resolve() const;

	  private:
		using Part = std::variant<std::string, lv_subject_t*>;

		std::vector<Part> m_parts; // First first
	};

	/**
	 * The subject called `name` as a widget built in `origin` would find it: in `origin` and its parents, then the
	 * globals and the global resolver
	 */
	lv_subject_t* lookupSubject(const lv_xml_subject_ctx_t* origin, std::string_view name);
} // namespace UI::Xml
