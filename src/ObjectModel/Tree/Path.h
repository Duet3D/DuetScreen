/*
 * Path.h
 *
 * The address of one node of the object model, as RepRapFirmware writes it: `heat.heaters[1].current`. A path is a
 * list of segments, each a field name or an array index. The empty path is the root.
 *
 * The same node can be reached from the JSON decoder's ids (`heat:heaters^:current` with the indices `[1]`), so a
 * path can be made from either. Its text form (str()) is the canonical name of the node: two paths to the same node
 * always give the same text, which is what OM bindings are keyed by.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace OM
{
	class Path
	{
	  public:
		/** One step of a path: a field name, or an array index */
		using Segment = std::variant<std::string, size_t>;

		/** The root */
		Path() = default;

		/**
		 * Read a path written as RepRapFirmware writes one, e.g. `heat.heaters[1].current` or `tools[0].offsets[2]`
		 * @return the path, or nothing if `text` isn't one (e.g. an unclosed bracket or an empty field name)
		 */
		static std::optional<Path> parse(std::string_view text);

		/**
		 * Make a path from a JSON decoder id, where ':' separates fields and each '^' stands for the next of `indices`,
		 * e.g. `heat:heaters^:current` with `[1]`
		 * @return the path, or nothing if the id has more '^' than `indices`
		 */
		static std::optional<Path> fromDecoderId(std::string_view id, std::span<const size_t> indices);

		/** The segments, root first */
		const std::vector<Segment>& segments() const { return m_segments; }

		/** Number of segments; 0 for the root */
		size_t size() const { return m_segments.size(); }

		/** Whether this is the root */
		bool empty() const { return m_segments.empty(); }

		/** This path followed by the field `name` */
		Path field(std::string_view name) const;

		/** This path followed by the index `index` */
		Path index(size_t index) const;

		/** This path followed by every segment of `tail` */
		Path operator/(const Path& tail) const;

		/** This path without its last segment; the root's parent is the root */
		Path parent() const;

		/** Whether `ancestor` is this path or one of its ancestors */
		bool startsWith(const Path& ancestor) const;

		/** The canonical text, e.g. `heat.heaters[1].current`; empty for the root */
		std::string str() const;

		/** Paths are equal if their segments are */
		bool operator==(const Path& other) const = default;

		/** Order by segments, so a sorted set of paths keeps each node just before its descendants */
		auto operator<=>(const Path& other) const = default;

	  private:
		std::vector<Segment> m_segments; // Root first
	};

	/**
	 * Whether the canonical path text `path` is `ancestor` or lies below it, judged at segment boundaries:
	 * `heat.heaters[1].current` is below `heat.heaters[1]`, but `heat.heaters[10]` is not.
	 */
	bool isAtOrBelow(std::string_view path, std::string_view ancestor);
} // namespace OM
