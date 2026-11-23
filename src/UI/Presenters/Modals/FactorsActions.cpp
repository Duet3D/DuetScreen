/*
 * FactorsActions.cpp
 */

#include "FactorsActions.h"
#include "Debug.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/FactorsSource.h"
#include "UI/Xml/View/Actions.h"
#include <optional>

namespace UI::FactorsActions
{
	namespace
	{
		/** The shared factor dialogs; nullptr, with a warning, while the data sources aren't running */
		Xml::FactorsSource* factors()
		{
			auto* source = Xml::DataSources::find<Xml::FactorsSource>();
			if (source == nullptr)
			{
				LOG_WARN("The factor dialogs' source isn't running");
			}
			return source;
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("factors.speed",
								"Open the dialog to change the speed factor",
								"tr.actions.factors_speed",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = factors())
									{
										source->openSpeed();
									}
								});
		Xml::Actions::addGlobal(
			"factors.extrusion",
			"Open the dialog to change extrusion factors: those of the tool numbered `arg`, if given, else every "
			"extruder's",
			"tr.actions.factors_extrusion",
			[](const Xml::ActionArgs& args)
			{
				if (auto* source = factors())
				{
					const auto tool = args.argInt();
					source->openExtrusion(tool && *tool >= 0 ? std::optional(static_cast<size_t>(*tool))
															 : std::nullopt);
				}
			});
		Xml::Actions::addGlobal("factors.preset",
								"Use a quick value for its factor. From a row of a factor's `presets`.",
								[](const Xml::ActionArgs& args)
								{
									const auto* preset = args.row<Xml::FactorPresetRow>();
									if (preset == nullptr || preset->factor == nullptr ||
										preset->factor->owner == nullptr)
									{
										LOG_WARN("actions.factors.preset must come from a row of a factor's `presets`");
										return;
									}
									preset->factor->owner->set(static_cast<size_t>(preset->factor->index.get()),
															   static_cast<float>(preset->percent));
								});
		Xml::Actions::addGlobal(
			"factors.extrusion_all",
			"Show every extruder's extrusion factor in its dialog (`arg` 1), or only the tool's it was opened for (0)",
			[](const Xml::ActionArgs& args)
			{
				if (auto* source = factors())
				{
					source->showAllExtruders(args.argInt().value_or(0) != 0);
				}
			});
		Xml::Actions::addGlobal("factors.set",
								"Use the slider's edited percentage (`value_confirmed`, dragged or typed) for its "
								"factor. From a row of a dialog's `factors`.",
								[](const Xml::ActionArgs& args)
								{
									const auto* row = args.row<Xml::FactorRow>();
									const auto value = args.value();
									if (row == nullptr || row->owner == nullptr || !value)
									{
										LOG_WARN("actions.factors.set must come from a row of a dialog's `factors`");
										return;
									}
									row->owner->set(static_cast<size_t>(row->index.get()), *value);
								});
	}
} // namespace UI::FactorsActions
