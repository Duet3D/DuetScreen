/*
 * JogActions.cpp
 */

#include "JogActions.h"
#include "UI/Xml/Data/JogSource.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <cmath>

namespace UI::JogActions
{
	namespace
	{
		/** The position of the choice row an action came from, or nullopt if it didn't come from one */
		std::optional<size_t> choiceOf(const Xml::ActionArgs& args)
		{
			const auto* row = args.row<Xml::JogChoiceRow>();
			return row ? std::optional(static_cast<size_t>(row->index.get())) : std::nullopt;
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("jog.distance",
								"Use that jog distance. From a row of `jog.distances`.",
								[](const Xml::ActionArgs& args)
								{
									if (const auto i = choiceOf(args))
									{
										Xml::JogSource::chooseDistance(*i);
									}
								});
		Xml::Actions::addGlobal("jog.feedrate",
								"Use that jog speed. From a row of `jog.feedrates`.",
								[](const Xml::ActionArgs& args)
								{
									if (const auto i = choiceOf(args))
									{
										Xml::JogSource::chooseFeedrate(*i);
									}
								});
		Xml::Actions::addGlobal(
			"jog.edit_distance",
			"Ask for a new value for that jog distance with a number pad, and use it. From a row of `jog.distances`.",
			[](const Xml::ActionArgs& args)
			{
				const auto i = choiceOf(args);
				const auto distances = StorageHelper::getData(ID_MOVE_DISTANCES);
				if (!i || *i >= distances.size())
				{
					return;
				}
				Xml::Prompts::askNumber({.header = _("move.distance_adjust_header", Units::getDisplayedDistanceUnit()),
										 .min = 0.01f,
										 .max = 1000.0f,
										 .value = distances[*i],
										 .onConfirm =
											 [i](float value)
										 {
											 Xml::JogSource::setDistance(*i, value);
											 Xml::JogSource::chooseDistance(*i);
										 },
										 .asker = nullptr});
			});
		Xml::Actions::addGlobal(
			"jog.edit_feedrate",
			"Ask for a new value for that jog speed with a number pad, and use it. From a row of `jog.feedrates`.",
			[](const Xml::ActionArgs& args)
			{
				const auto i = choiceOf(args);
				const auto feedrates = StorageHelper::getData(ID_MOVE_FEEDRATES);
				if (!i || *i >= feedrates.size())
				{
					return;
				}
				Xml::Prompts::askNumber({.header = _("move.feedrate_adjust_header", Units::getDisplayedSpeedUnit()),
										 .min = 0.01f,
										 .max = 1000.0f,
										 .value = static_cast<float>(feedrates[*i]),
										 .onConfirm =
											 [i](float value)
										 {
											 Xml::JogSource::setFeedrate(*i, static_cast<uint32_t>(std::lround(value)));
											 Xml::JogSource::chooseFeedrate(*i);
										 },
										 .asker = nullptr});
			});
	}
} // namespace UI::JogActions
