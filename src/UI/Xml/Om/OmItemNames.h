/*
 * OmItemNames.h
 *
 * The names relative to one object model item, given to a row's scope as its `row.*` names (see Binding/Names.h): in a
 * row of `<ds_list list="om.heat.heaters">`, `row.current` is `om.heat.heaters[<row>].current`, `row.model.pid.p` and
 * `row.active[{row.index}]` work the same way, and an array of the item is a list (`<ds_list list="row.heaters">`).
 * `row.value|int` (any converter) converts the item itself. Every field is the item's, whether or not the printer has
 * sent it yet. This resolver is given the names without their `row.`.
 */

#pragma once

#include "OmPath.h"
#include "UI/Xml/Binding/Resolver.h"
#include <optional>
#include <string_view>

namespace UI::Xml
{
	class OmBindings;

	class OmItemNames : public Resolver
	{
	  public:
		/** Names relative to `item`, from `bindings` */
		void setItem(OmBindings& bindings, OmBoundPath item);

		/** A field of the item (see the file comment) */
		Subject* findSubject(const lv_xml_subject_ctx_t* origin,
							 std::string_view name,
							 std::string_view written) override;

		/** An array of the item (see the file comment) */
		ListBase* findList(const lv_xml_subject_ctx_t* origin,
						   std::string_view name,
						   std::string_view written) override;

	  private:
		/**
		 * The path a relative name's path stands for, its `{name}` indices looked up from `origin`
		 * @return the path, or nothing if an index's name isn't found
		 */
		std::optional<OmBoundPath> resolve(const lv_xml_subject_ctx_t* origin, std::string_view relative) const;

		OmBindings* m_bindings = nullptr; // Where names come from; nullptr until setItem()
		OmBoundPath m_item;				  // The item
	};
} // namespace UI::Xml
