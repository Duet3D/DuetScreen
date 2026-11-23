/*
 * SelectorActions.cpp
 */

#include "SelectorActions.h"
#include "Actions.h"
#include "Debug.h"
#include "ValueSelector.h"

namespace UI::Xml::SelectorActions
{
	namespace
	{
		/** The selector `arg` names; nullptr, with a warning, if there is none */
		ValueSelector* named(const ActionArgs& args)
		{
			auto* selector = ValueSelector::find(args.arg);
			if (selector == nullptr)
			{
				LOG_WARN("{:s}: no selector called '{:s}'", args.name, args.arg);
			}
			return selector;
		}

		/** The choices row the action was raised in; nullptr, with a warning, if it wasn't in one */
		const ValueChoiceRow* choiceOf(const ActionArgs& args)
		{
			const auto* row = args.row<ValueChoiceRow>();
			if (row == nullptr || row->owner == nullptr)
			{
				LOG_WARN("{:s} must come from a row of a selector's `choices`", args.name);
				return nullptr;
			}
			return row;
		}
	} // namespace

	void registerAll()
	{
		Actions::addGlobal("selector.decrement",
						   "Step the selector named by `arg` (its id, e.g. `jog.distance`) down",
						   [](const ActionArgs& args)
						   {
							   if (auto* selector = named(args))
							   {
								   selector->step(-1);
							   }
						   });
		Actions::addGlobal("selector.increment",
						   "Step the selector named by `arg` (its id) up",
						   [](const ActionArgs& args)
						   {
							   if (auto* selector = named(args))
							   {
								   selector->step(1);
							   }
						   });
		Actions::addGlobal("selector.edit",
						   "Ask for a value for the selector named by `arg` (its id) with a number pad",
						   [](const ActionArgs& args)
						   {
							   if (auto* selector = named(args))
							   {
								   selector->edit();
							   }
						   });
		Actions::addGlobal("selector.pick",
						   "Use the preset. From a row of a selector's `choices`.",
						   [](const ActionArgs& args)
						   {
							   if (const auto* row = choiceOf(args))
							   {
								   row->owner->pick(static_cast<size_t>(row->index.get()));
							   }
						   });
		Actions::addGlobal("selector.edit_choice",
						   "Ask for a new value for the preset with a number pad, and use it. From a row of a "
						   "selector's `choices`.",
						   [](const ActionArgs& args)
						   {
							   if (const auto* row = choiceOf(args))
							   {
								   row->owner->editPreset(static_cast<size_t>(row->index.get()));
							   }
						   });
	}
} // namespace UI::Xml::SelectorActions
