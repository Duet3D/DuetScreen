/*
 * List.h
 *
 * A list is a dynamic-length sequence of rows. Each row owns its own subjects, and may own lists of its own (a tool
 * row has a list of heaters). XML shows a list with
 *
 *     <ds_list list="tools" item="tool_card"/>
 *
 * which builds one `tool_card` per row, each in a scope holding that row's fields. Lists tell the views showing them
 * when they grow or shrink; views add or remove rows at the end, so existing rows, and their scroll position, focus
 * and in-progress edits, survive a length change.
 */

#pragma once

#include "Fields.h"
#include "Subjects.h"
#include <concepts>
#include <memory>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** One entry of a List. Subclasses add subjects (and nested lists) and declare them in `fields()`. */
	class Row : public FieldProvider
	{
	  public:
		/** Declare `index`, which every row has, then the row's own fields */
		void allFields(FieldVisitor& f)
		{
			f.field("index", index, "Position in the list, from 0");
			fields(f);
		}

		IntSubject index; // Position in the list, available to XML as `index`
	};

	/** The type-independent part of List<R>, used by the views that show lists */
	class ListBase
	{
	  public:
		/** Something that shows a list: told when the list changes length */
		class Listener
		{
		  public:
			virtual ~Listener() = default;

			/** Rows were added (after they are filled in), or are about to be removed (before they are destroyed) */
			virtual void onListResized(size_t oldSize, size_t newSize) = 0;

			/** The list is being destroyed; let go of every row */
			virtual void onListDestroyed() = 0;
		};

		ListBase() = default;

		/** Tells listeners the list is going away */
		virtual ~ListBase();

		ListBase(const ListBase&) = delete;
		ListBase& operator=(const ListBase&) = delete;

		/** Number of rows */
		virtual size_t size() const = 0;

		/** The row at `index`, which must be less than size() */
		virtual Row& row(size_t index) = 0;

		/** Declare the fields every row has (see Row::allFields()), whether or not the list has rows */
		virtual void rowFields(FieldVisitor& f) const = 0;

		/** What the list holds, as the UI API reference names it: "list" */
		virtual std::string_view typeName() const { return "list"; }

		/** Start telling `listener` about length changes. Adding twice has no effect. */
		void addListener(Listener* listener);

		/** Stop telling `listener` about length changes */
		void removeListener(Listener* listener);

	  protected:
		/** Tell listeners the length changed from `oldSize` to `newSize` */
		void notifyResized(size_t oldSize, size_t newSize);

		/** Tell listeners the list is going away, and forget them */
		void notifyDestroyed();

	  private:
		std::vector<Listener*> m_listeners; // Views showing this list
	};

	/**
	 * A list of rows of type R
	 * @tparam R Row type; default constructible, declares its fields in `fields()`
	 */
	template <typename R>
		requires std::derived_from<R, Row> && std::default_initializable<R>
	class List : public ListBase
	{
	  public:
		/** Views must let go of the rows before the rows' subjects are destroyed */
		~List() override { notifyDestroyed(); }

		/** Number of rows */
		size_t size() const override { return m_rows.size(); }

		/** The row at `index`, which must be less than size() */
		R& row(size_t index) override { return *m_rows.at(index); }

		/** The row at `index`, which must be less than size() */
		R& operator[](size_t index) { return *m_rows.at(index); }

		/** Declare the fields of an R, from a row made for the purpose */
		void rowFields(FieldVisitor& f) const override
		{
			R example;
			example.allFields(f);
		}

		/**
		 * Make the list `count` rows long and call `update(row, index)` on every row. New rows are filled in before
		 * views are told about them, so they never show default values; removed rows leave the views first.
		 */
		template <typename F>
			requires std::invocable<F, R&, size_t>
		void sync(size_t count, F&& update)
		{
			const size_t oldSize = m_rows.size();
			if (count < oldSize)
			{
				notifyResized(oldSize, count);
				m_rows.resize(count);
			}

			for (size_t i = 0; i < count; i++)
			{
				if (i == m_rows.size())
				{
					m_rows.push_back(std::make_unique<R>());
					m_rows.back()->index.set(static_cast<int32_t>(i));
				}
				update(*m_rows[i], i);
			}

			if (count > oldSize)
			{
				notifyResized(oldSize, count);
			}
		}

		/** Remove every row */
		void clear()
		{
			sync(0, [](R&, size_t) {});
		}

	  private:
		std::vector<std::unique_ptr<R>> m_rows; // unique_ptr: subjects are bound by address and must never move
	};
} // namespace UI::Xml
