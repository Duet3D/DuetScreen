/*
 * SimulatorSettings.cpp
 */

#include "SimulatorSettings.h"
#include "Debug.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/SettingsSource.h"
#include "UI/Xml/Data/SimulatorSource.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
#if DUET_SIMULATOR
#  include "Simulator/Simulator.h"
#endif

namespace UI::SimulatorSettings
{
	namespace
	{
		/** A setting's name and what its value means, the same in builds with and without the simulated Duet */
		struct Named
		{
			const char* name;		 // `source.settings.<name>`
			const char* description; // For the UI API reference
		};
		constexpr Named PRESET{
			"simulator_preset",
			"Index of the preset the simulated Duet started from; choosing one makes it that machine, "
			"replacing its parts"};
		constexpr Named NAME{"simulator_name", "The simulated Duet's name (network.name)"};
		constexpr Named MODE{"simulator_mode", "Index of the simulated Duet's mode: FFF, CNC or laser"};
		constexpr Named STATUS{"simulator_status",
							   "Index of the status the simulated Duet reports whatever it is doing; 0 for what it is "
							   "doing"};
		constexpr Named RESPONDING{
			"simulator_responding",
			"1 if the simulated Duet answers requests; if not, the screen times out and reconnects"};
		constexpr Named PLUGGED_IN{"simulator_plugged_in",
								   "1 if the simulated Duet can be connected to; unplugged, it answers nothing"};
		constexpr Named REPLY_DELAY{"simulator_reply_delay", "ms the simulated Duet takes to answer each request"};
		constexpr Named NOISE{"simulator_noise",
							  "Noise on the simulated Duet's temperatures, tenths of °C (edited in °C)"};

		/** Its temperatures' noise is edited in °C, kept in tenths */
		constexpr int32_t NOISE_SCALE = 10;

		/** The statuses that can be forced, after "whatever it is doing" (index 0), as state.status names them */
		constexpr std::array FORCED_STATUSES = {"idle",
												"busy",
												"processing",
												"paused",
												"pausing",
												"resuming",
												"cancelling",
												"changingTool",
												"simulating",
												"starting",
												"updating",
												"halted",
												"off"};

		/** The machine modes, in Sim::MachineMode's order */
		constexpr std::array MODES = {"simulator.modes.fff", "simulator.modes.cnc", "simulator.modes.laser"};

		/** The preset names' translation keys */
		std::vector<std::string> presetNames()
		{
			std::vector<std::string> names;
#if DUET_SIMULATOR
			for (const Sim::Preset& preset : Sim::presets())
			{
				names.push_back(_("simulator.presets." + std::string(preset.key)));
			}
#endif
			return names;
		}

		/** The forced statuses' names, translated, after "whatever it is doing" */
		std::vector<std::string> statusNames()
		{
			std::vector<std::string> names{_("simulator.as_it_is")};
			for (const char* status : FORCED_STATUSES)
			{
				names.push_back(_("state." + std::string(status)));
			}
			return names;
		}

		/** The modes' names, translated */
		std::vector<std::string> modeNames()
		{
			std::vector<std::string> names;
			for (const char* mode : MODES)
			{
				names.push_back(_(mode));
			}
			return names;
		}

#if DUET_SIMULATOR
		/** Show the simulated Duet's parts again, after a change to the whole machine */
		void showParts()
		{
			if (auto* source = Xml::DataSources::find<Xml::SimulatorSource>())
			{
				source->update();
			}
		}

		/** Change how the simulated Duet behaves with `change` */
		template <typename F>
		void behave(F&& change)
		{
			Sim::Behaviour behaviour = Sim::Simulator::get().behaviour();
			change(behaviour);
			Sim::Simulator::get().setBehaviour(behaviour);
		}

		/** Change the simulated Duet's configuration with `change` */
		template <typename F>
		void configure(F&& change)
		{
			Sim::Config config = Sim::Simulator::get().config();
			change(config);
			Sim::Simulator::get().setConfig(config);
			showParts();
		}
#endif
	} // namespace

#if DUET_SIMULATOR
	void registerAll()
	{
		Xml::Settings::add({.name = PRESET.name,
							.description = PRESET.description,
							.read =
								[]
							{
								const std::string preset = Sim::Simulator::get().config().preset;
								const auto& presets = Sim::presets();
								const auto it = std::ranges::find_if(
									presets, [&preset](const Sim::Preset& p) { return p.key == preset; });
								return it == presets.end() ? -1 : static_cast<int32_t>(it - presets.begin());
							},
							.write =
								[](int32_t index)
							{
								const auto& presets = Sim::presets();
								if (index >= 0 && index < static_cast<int32_t>(presets.size()))
								{
									Sim::Simulator::get().setConfig(presets[static_cast<size_t>(index)].make());
									showParts();
								}
							},
							.options = [] { return Xml::Settings::lines(presetNames()); }});
		Xml::Settings::add(Xml::TextSetting{.name = NAME.name,
											.description = NAME.description,
											.read = [] { return Sim::Simulator::get().config().name; },
											.write = [](std::string_view name)
											{ configure([name](Sim::Config& config) { config.name = name; }); }});
		Xml::Settings::add({.name = MODE.name,
							.description = MODE.description,
							.read = [] { return static_cast<int32_t>(Sim::Simulator::get().config().mode); },
							.write =
								[](int32_t index)
							{
								configure([index](Sim::Config& config)
										  { config.mode = static_cast<Sim::MachineMode>(std::clamp(index, 0, 2)); });
							},
							.options = [] { return Xml::Settings::lines(modeNames()); }});
		Xml::Settings::add({.name = STATUS.name,
							.description = STATUS.description,
							.read =
								[]
							{
								const auto forced = Sim::Simulator::get().forcedStatus();
								if (!forced)
								{
									return 0;
								}
								const auto it = std::ranges::find(FORCED_STATUSES, *forced);
								return it == FORCED_STATUSES.end()
										   ? 0
										   : static_cast<int32_t>(it - FORCED_STATUSES.begin()) + 1;
							},
							.write =
								[](int32_t index)
							{
								Sim::Simulator::get().forceStatus(
									index > 0 && index <= static_cast<int32_t>(FORCED_STATUSES.size())
										? std::optional<std::string>(FORCED_STATUSES[static_cast<size_t>(index - 1)])
										: std::nullopt);
							},
							.options = [] { return Xml::Settings::lines(statusNames()); }});
		Xml::Settings::add(
			{.name = RESPONDING.name,
			 .description = RESPONDING.description,
			 .read = [] { return Sim::Simulator::get().behaviour().responding ? 1 : 0; },
			 .write = [](int32_t on) { behave([on](Sim::Behaviour& behaviour) { behaviour.responding = on != 0; }); }});
		Xml::Settings::add(
			{.name = PLUGGED_IN.name,
			 .description = PLUGGED_IN.description,
			 .read = [] { return Sim::Simulator::get().behaviour().pluggedIn ? 1 : 0; },
			 .write = [](int32_t on) { behave([on](Sim::Behaviour& behaviour) { behaviour.pluggedIn = on != 0; }); }});
		Xml::Settings::add(
			{.name = REPLY_DELAY.name,
			 .description = REPLY_DELAY.description,
			 .read = [] { return Sim::Simulator::get().behaviour().replyDelayMs; },
			 .write = [](int32_t ms)
			 { behave([ms](Sim::Behaviour& behaviour) { behaviour.replyDelayMs = std::max(0, ms); }); }});
		Xml::Settings::add(
			{.name = NOISE.name,
			 .description = NOISE.description,
			 .read = []
			 { return static_cast<int32_t>(std::lround(Sim::Simulator::get().behaviour().noise * NOISE_SCALE)); },
			 .write =
				 [](int32_t tenths)
			 {
				 behave([tenths](Sim::Behaviour& behaviour)
						{ behaviour.noise = static_cast<float>(std::max(0, tenths)) / NOISE_SCALE; });
			 },
			 .scale = NOISE_SCALE});
	}
#else
	void registerAll()
	{
		// The same names as builds with the simulated Duet have, reading 0 and doing nothing
		const auto none = [](const Named& named, std::function<std::string()> options = nullptr) -> Xml::Setting
		{
			return {.name = named.name,
					.description = named.description,
					.read = [] { return 0; },
					.write = [name = std::string(named.name)](int32_t)
					{ LOG_WARN("settings.{:s}: this build has no simulated Duet", name); },
					.options = std::move(options)};
		};
		Xml::Settings::add(none(PRESET, [] { return Xml::Settings::lines(presetNames()); }));
		Xml::Settings::add(Xml::TextSetting{.name = NAME.name,
											.description = NAME.description,
											.read = [] { return std::string(); },
											.write = [](std::string_view) {}});
		Xml::Settings::add(none(MODE, [] { return Xml::Settings::lines(modeNames()); }));
		Xml::Settings::add(none(STATUS, [] { return Xml::Settings::lines(statusNames()); }));
		Xml::Settings::add(none(RESPONDING));
		Xml::Settings::add(none(PLUGGED_IN));
		Xml::Settings::add(none(REPLY_DELAY));
		Xml::Setting noise = none(NOISE);
		noise.scale = NOISE_SCALE;
		Xml::Settings::add(noise);
	}
#endif
} // namespace UI::SimulatorSettings
