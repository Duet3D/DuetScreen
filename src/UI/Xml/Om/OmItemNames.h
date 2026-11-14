/*
 * OmItemNames.h
 *
 * The names relative to one object model item: in a row of `<ds_list list="om.heat.heaters">`, `current` is
 * `om.heat.heaters[<row>].current`, `model.pid.p` and `active[{index}]` work the same way, and an array of the item is
 * a list (`<ds_list list="heaters">`). `value|int` (any converter) converts the item itself.
 *
 * An item's own fields, those the printer has sent and those the schema says it can send (ObjectModel/Tree/Schema.h),
 * are found before names outside (a resolver, see Binding/Resolver.h, asked right after the scope's own names); any
 * other field is still found, as the last chance, if nothing else has the name.
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
		Subject* findSubject(const lv_xml_subject_ctx_t* origin, std::string_view name, bool last) override;

		/** An array of the item (see the file comment) */
		ListBase* findList(const lv_xml_subject_ctx_t* origin, std::string_view name, bool last) override;

	  private:
		/**
		 * The path a relative name's path stands for, its `{name}` indices looked up from `origin`
		 * @param last Whether any field will do; otherwise the first field must be one of the item's (see the file
		 *        comment)
		 * @return the path, or nothing if it isn't the item's
		 */
		std::optional<OmBoundPath> resolve(const lv_xml_subject_ctx_t* origin, std::string_view relative, bool last) const;

		OmBindings* m_bindings = nullptr; // Where names come from; nullptr until setItem()
		OmBoundPath m_item;				  // The item
	};
} // namespace UI::Xml
