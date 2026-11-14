/*
 * DsScope.h
 *
 * `<ds_scope path="om.heat.heaters[{value}]" item="heater_detail"/>` builds one `item` component in a scope where
 * names are relative to the object model element at `path`, as in a row of an object model list (see
 * UI/Xml/Om/OmItemNames.h): `current`, `model.pid.p`, `heaters` as a list. So a component shows one element without
 * repeating its path, and when an index of the path is a subject's value, the names follow the element it points at
 * now, without rebuilding anything.
 */

#pragma once

#include "UI/Xml/Binding/Scope.h"
#include "UI/Xml/Om/OmItemNames.h"
#include "helix_xml.h"
#include <memory>
#include <string>

namespace UI::Xml
{
	class DsScope
	{
	  public:
		/**
		 * Build `item` in `container`, its names relative to `element`
		 * @param parentContext Context the ds_scope is built in; the item's scope is its child
		 */
		DsScope(lv_obj_t* container,
				OmBindings& bindings,
				OmBoundPath element,
				const std::string& item,
				lv_xml_subject_ctx_t* parentContext);

		/** Delete the item's widgets, then their scope; runs while LVGL deletes the container */
		~DsScope();

		DsScope(const DsScope&) = delete;
		DsScope& operator=(const DsScope&) = delete;

		/** Register `<ds_scope>` with the XML engine */
		static void registerXml();

	  private:
		/** `<ds_scope>` create callback: a plain container */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_scope>` apply callback: lv_obj attributes, then build the item */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		OmItemNames m_names;		   // The names relative to the element; outlives m_scope, which uses it
		std::unique_ptr<Scope> m_scope; // Where the item is built
		lv_obj_t* m_obj = nullptr;	   // The item's root widget, or nullptr if it couldn't be built
	};
} // namespace UI::Xml
