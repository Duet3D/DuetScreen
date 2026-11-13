/*
 * PlacementScope.h
 *
 * The names a widget placed by `<ds_layout>` can use besides its presenter's and the globals: how much room its place
 * gives it, as size classes, so a component can fit what it shows to its size.
 *
 *     <!-- The standby target goes when the row is too narrow for it -->
 *     <ds_button name="standby" bind_text="standby">
 *         <bind_flag_if cond="widget.width_class eq 0" flag="hidden"/>
 *     </ds_button>
 *     <!-- A bigger gauge shows its value in a larger font -->
 *     <lv_label name="value" bind_text="text">
 *         <style name="widget_value"/>
 *         <bind_style_if name="widget_value_large" cond="widget.size_class eq 1"/>
 *         <bind_style_if name="widget_value_huge" cond="widget.size_class eq 2"/>
 *     </lv_label>
 *
 * Each of the place's width and height is compact (0), regular (1) or expanded (2), and `widget.size_class` is the
 * smaller of the two (for something that needs both, such as an arc), by the active theme's breakpoints,
 * the consts `widget_regular_min` (a length below it is compact) and `widget_expanded_min` (a length from it is
 * expanded), so XML never compares pixels and a theme with larger fonts moves every breakpoint at once. A theme
 * without them has no breakpoints: everything is regular.
 *
 * Only a grid measures its widgets, and it measures the cells it gives them, which the grid alone sizes, never the
 * widget's content: so what a widget hides or restyles for its size can't change that size, and the layout can't
 * loop. A strip sizes its widgets by their content, so its widgets are always regular, regular, regular, landscape.
 *
 * Lay a widget out with `%`, `fr` and `flex_grow` first: it must look right at its default and smallest sizes without
 * these. Use them to hide secondary parts, to step a value's font, and for orientation, with `bind_flag_if` and
 * `bind_style_if` (never `<if>`, which builds its contents again at every change), and say why in a comment.
 *
 * Its names are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md) under Layouts.
 */

#pragma once

#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/Scope.h"
#include "UI/Xml/Binding/Subjects.h"
#include "helix_xml.h"
#include <cstddef>
#include <cstdint>

namespace UI::Xml
{
	class PlacementScope : public FieldProvider
	{
	  public:
		/** How much room one dimension of a place gives, as `widget.width_class` and `widget.height_class` hold it */
		enum class SizeClass : int32_t
		{
			Compact = 0,  // Below the theme's `widget_regular_min`
			Regular = 1,  // Between the breakpoints
			Expanded = 2, // From the theme's `widget_expanded_min`
		};

		/** A scope followed only by the globals, for one placed widget: regular, regular, landscape until it follows a place */
		PlacementScope();

		/** Stop following the place and the theme; the subjects go, which unbinds every widget still bound to them */
		~PlacementScope() override;

		PlacementScope(const PlacementScope&) = delete;
		PlacementScope& operator=(const PlacementScope&) = delete;

		/** Declare the placed widget's names */
		void fields(FieldVisitor& f) override;

		/**
		 * Measure `place`, a box sized by its parent's layout alone (a grid's cells), and follow its size, and the
		 * theme's breakpoints, until it is deleted or this scope goes, whichever is first
		 */
		void follow(lv_obj_t* place);

		/** The scope, to be the parent of the placed view's own (see XmlView) */
		const Scope& getScope() const { return m_scope; }

		/** The class of a length in pixels, by the active theme's breakpoints */
		static SizeClass classify(int32_t length);

	  private:
		/** The place's size changed: classify it again */
		static void sizeChangedCb(lv_event_t* e);

		/** The place is being deleted: stop measuring it */
		static void placeDeletedCb(lv_event_t* e);

		/** Stop following the place, if there is one */
		void unfollow();

		/** Publish the classes of the place's size */
		void measure();

		Scope m_scope;			   // Holds the subjects below
		lv_obj_t* m_place = nullptr; // The box measured; nullptr before follow() and once it is deleted
		size_t m_themeListener;	   // Our id with the theme (see UI::Themes::addThemeListener)
		IntSubject m_widthClass{static_cast<int32_t>(SizeClass::Regular)};  // SizeClass of the place's width
		IntSubject m_heightClass{static_cast<int32_t>(SizeClass::Regular)}; // SizeClass of the place's height
		IntSubject m_sizeClass{static_cast<int32_t>(SizeClass::Regular)};  // The smaller of the two classes
		IntSubject m_portrait;	   // 1 if the place is taller than it is wide, else 0
	};
} // namespace UI::Xml
