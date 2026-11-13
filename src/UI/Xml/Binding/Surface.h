/*
 * Surface.h
 *
 * A value over an X/Y area, such as a bed's height map, for <ds_heatmap> to show as a colour map:
 *
 *     <ds_heatmap bind_surface="surface"/>
 *
 * The values are a grid of samples, so the work of finding them (e.g. interpolating a height map) is done by the
 * presenter, off the UI thread, and the widget only colours them. Sample (i, j), for i < columns and j < rows, is the
 * value at
 *
 *     x = x.min + i * (x.max - x.min) / columns,   y = y.min + j * (y.max - y.min) / rows
 *
 * so j = 0 is the lowest y. NaN marks a sample with no value, left uncoloured.
 */

#pragma once

#include "Subjects.h"
#include <cstddef>
#include <vector>

namespace UI::Xml
{
	class Surface : public Subject
	{
	  public:
		/** A span of x, y or values */
		struct Range
		{
			float min = 0.0f; // Lowest
			float max = 0.0f; // Highest
		};

		/** What a surface shows */
		struct Values
		{
			Range x{0.0f, 100.0f};		// The area's X span
			Range y{0.0f, 100.0f};		// The area's Y span
			Range scale{-1.0f, 1.0f};	// The values mapped to the colour scale; outside it they are clamped
			size_t columns = 0;			// Samples along X
			size_t rows = 0;			// Samples along Y
			std::vector<float> samples; // columns x rows samples, row by row from the lowest y; NaN for none
		};

		/** Create an empty surface over 0-100 x 0-100 */
		Surface();

		/** Show `values`, notifying the widgets bound to it */
		void set(Values values);

		/** Show nothing over 0-100 x 0-100 */
		void clear();

		/** What it shows */
		const Values& get() const { return m_values; }

		/** The value of sample (i, j), or NaN if it has none or is outside the grid */
		float sample(size_t i, size_t j) const;

		/** The LVGL subject that widgets bind to: a pointer subject pointing at this Surface */
		lv_subject_t* raw() override { return m_subject; }

		/** "surface" */
		std::string_view typeName() const override { return "surface"; }

	  private:
		Values m_values;	 // What it shows
		LvSubject m_subject; // Pointer subject whose value is this Surface
	};
} // namespace UI::Xml
