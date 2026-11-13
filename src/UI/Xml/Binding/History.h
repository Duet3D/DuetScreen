/*
 * History.h
 *
 * A history is a list of series, each a name and its recent values, all sampled together at a fixed rate. Charts
 * show one with
 *
 *     <ds_graph history="temperatures" seconds="60"/>
 *
 * A subject holds only the current value; a history keeps the past. Data sources record into a history whether or
 * not anything shows it, so a chart built later (on a page opened later, or rebuilt by a hot reload) starts with
 * what happened before it existed.
 *
 * A history is a List (see List.h): its rows are the series, and `sync()` adds, removes and renames them, so a
 * `<ds_list>` can show the series too (e.g. a legend). `record()` then adds one value to every series.
 */

#pragma once

#include "List.h"
#include <cstdint>
#include <deque>
#include <type_traits>

namespace UI::Xml
{
	/** One series of a History */
	class SeriesRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		/** Recorded values, oldest first; the newest was recorded by the latest History::record() */
		const std::deque<int32_t>& values() const { return m_values; }

		StringSubject<> name; // Name shown for the series, e.g. in a legend

	  private:
		friend class History;

		std::deque<int32_t> m_values; // Recorded values, oldest first
	};

	class History : public List<SeriesRow>
	{
	  public:
		/** Something that shows a history: told when values are recorded */
		class SampleListener
		{
		  public:
			virtual ~SampleListener() = default;

			/** One value was added to every series */
			virtual void onSampled() = 0;
		};

		/**
		 * @param samplesPerSecond How often record() is called
		 * @param capacity Values kept per series; older ones are dropped
		 */
		History(uint32_t samplesPerSecond, size_t capacity);

		/** How often values are recorded */
		uint32_t samplesPerSecond() const { return m_samplesPerSecond; }

		/** Values kept per series */
		size_t capacity() const { return m_capacity; }

		/** "history": shown with `<ds_graph history="…">`, or as a list of its series */
		std::string_view typeName() const override { return "history"; }

		/**
		 * Add one value to every series, then tell the sample listeners. A series added since the last call has fewer
		 * values than the others; the newest values of every series line up.
		 * @param value `value(row, index)` gives the value to add to each series
		 */
		template <typename F>
			requires std::is_invocable_r_v<int32_t, F, SeriesRow&, size_t>
		void record(F&& value)
		{
			for (size_t i = 0; i < size(); i++)
			{
				SeriesRow& series = row(i);
				series.m_values.push_back(value(series, i));
				while (series.m_values.size() > m_capacity)
				{
					series.m_values.pop_front();
				}
			}
			notifySampled();
		}

		/** Start telling `listener` about recorded values. Adding twice has no effect. Length changes and the
		 * history's destruction are told to ListBase listeners (see addListener). */
		void addSampleListener(SampleListener* listener);

		/** Stop telling `listener` about recorded values */
		void removeSampleListener(SampleListener* listener);

	  private:
		/** Tell the sample listeners a value was recorded */
		void notifySampled();

		uint32_t m_samplesPerSecond;					// How often record() is called
		size_t m_capacity;								// Values kept per series
		std::vector<SampleListener*> m_sampleListeners; // Views showing this history
	};
} // namespace UI::Xml
