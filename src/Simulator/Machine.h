/*
 * Machine.h
 *
 * The simulated machine: built from a Config, it runs G-code (MachineGcode.cpp) and moves on with time (advance()):
 * heaters trend towards their targets, their readings a little noisy once there, axes travel to where they were sent, a job prints, a
 * pause or a cancel takes a moment. It reports itself as a Duet's object model (MachineModel.cpp), answers the
 * requests a Duet answers (M409, M20, M36, M36.1, M36.2) and counts a seq for each key of the object model that
 * changes, so the screen asks for what changed, as it does of a real Duet. Its SD card (MachineFiles.cpp) holds its
 * jobs, with thumbnails, a few macros and filaments, and heightmaps made for its bed.
 *
 * Not thread safe: the Simulator owns it and locks around it. Time only passes when advance() is called, so a test
 * can run it step by step.
 */

#pragma once

#include "Config.h"
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Sim
{
	/** JSON whose fields keep the order they were added in, as a Duet sends them (the screen's decoder cares) */
	using Json = nlohmann::ordered_json;

	/** A message box (M291), as state.messageBox reports it */
	struct MessageBox
	{
		int32_t seq = 0;					 // Changes with every new box
		int32_t mode = 0;					 // 0-7, as M291 S
		std::string title{};					 // Its title
		std::string message{};				 // Its text
		bool cancelButton = false;			 // Has a Cancel button
		float timeout = 0.0f;				 // Closes itself after this many seconds; 0 never
		int32_t axisControls = 0;			 // Bitmap of the axes it offers to jog (mode 2 and 3)
		std::vector<std::string> choices{};	 // Mode 4: the choices
		std::optional<float> min{};			 // Modes 5-7: the lowest value or length
		std::optional<float> max{};			 // Modes 5-7: the highest
		std::optional<Json> defaultValue{};	 // Modes 4-7: the value offered
	};

	/** The simulated machine. See the file comment. */
	class Machine
	{
	  public:
		/** A machine built from `config`, idle, cold, at its axes' lowest positions */
		explicit Machine(const Config& config);

		/** Become `config`, keeping what still applies (temperatures, positions, a job running, the tool in use) */
		void reconfigure(const Config& config);

		/** What the machine is */
		const Config& config() const { return m_config; }

		/** Whether G-code changed the configuration (M30 deleted a job) since this was last asked */
		bool takeConfigChange() { return std::exchange(m_configChanged, false); }

		/** Let `seconds` pass: heaters heat and cool, axes travel, a job prints */
		void advance(double seconds);

		/**
		 * Act on G-code, one or more lines, each perhaps with a line number and checksum, as a Duet does
		 * @return the replies to send back, each a JSON object: e.g. an M409 result, and last, any text replied, as
		 *         {"resp":"..."}
		 */
		std::vector<Json> execute(std::string_view gcode);

		/** The whole object model, every field */
		Json objectModel() const;

		/** Noise added to each temperature reading of a heater holding its temperature, °C (its standard deviation); 0
		 * for none */
		void setNoise(float noise) { m_noise = noise; }

		/** Force the status the machine reports, e.g. "halted"; nothing to report what it is doing */
		void forceStatus(std::optional<std::string> status) { m_forcedStatus = std::move(status); }

		/** The status forced, if any */
		const std::optional<std::string>& forcedStatus() const { return m_forcedStatus; }

		/** Show a message box of `mode` (0-7) with example content, as M291 would */
		void showMessageBox(int32_t mode);

		/** The status the machine reports (state.status) */
		std::string status() const;

		/** The temperature heater `heater` reads now, °C */
		float temperature(size_t heater) const;

		/** The extruder the tool in use extrudes with and loads filament into (its first), if it has one */
		std::optional<size_t> toolExtruder() const;

		/** Axis `axis`'s position, mm, in machine coordinates */
		float position(size_t axis) const;

	  private:
		/** A heater's state */
		struct HeaterState
		{
			float current = 0.0f;  // Its true temperature, °C
			float reading = 0.0f;  // What it reads: current plus noise
			float active = 0.0f;   // Active temperature
			float standby = 0.0f;  // Standby temperature
			std::string state = "off"; // off, standby, active or fault
			float pwm = 0.0f;	   // Power, 0-1
		};

		/** A tool's state */
		struct ToolState
		{
			std::string state = "off"; // off, standby or active
			int32_t spindleRpm = 0;	   // Speed its spindle is set to (M568 F)
		};

		/** An axis's state */
		struct AxisState
		{
			float position = 0.0f;					// Machine position now
			float target = 0.0f;					// Where it is going
			bool homed = false;						// Homed
			float babystep = 0.0f;					// Babystepping (Z)
			std::vector<float> workplaceOffsets{};	// One per workplace (G54 to G59.3)
		};

		/** An extruder's state */
		struct ExtruderState
		{
			float position = 0.0f;	   // Total extruded, mm
			float factor = 1.0f;	   // Extrusion factor (M221)
			std::string filament{};	   // Filament loaded (M701)
		};

		/** A fan's state */
		struct FanState
		{
			float requested = 0.0f; // Speed asked for, 0-1
			float actual = 0.0f;	// Speed it runs at, 0-1
		};

		/** A spindle's state */
		struct SpindleState
		{
			int32_t active = 0;			   // Speed set, rpm
			int32_t current = 0;		   // Speed it turns at, rpm
			std::string state = "stopped"; // stopped, forward or reverse
		};

		/** The job being printed, or the last one */
		struct JobState
		{
			bool running = false;			 // A job is printing, paused or cancelling
			size_t file = 0;				 // Its index in the configuration's files
			double duration = 0.0;			 // Time since it started, s
			double warmUp = 0.0;			 // Time spent heating before printing, s
			double pauseDuration = 0.0;		 // Time spent paused, s
			double progress = 0.0;			 // How far through, 0-1
			bool warming = false;			 // Heating before printing
			std::vector<bool> cancelled{};	 // Each object's cancelled state (M486)
			std::string lastFileName{};		 // The last job's path, once one has finished
			double lastDuration = 0.0;		 // How long it took, s
		};

		/** What the machine is busy with for a moment before its status changes, e.g. pausing */
		struct Transition
		{
			std::string status{};	  // The status while it lasts, e.g. "pausing"
			double remaining = 0; // Seconds left
			std::string then{};	  // What follows: "paused", "cancelled" (the job ends), or "" (carry on)
		};

		// MachineGcode.cpp

		/** A G or M code with its parameters */
		struct Command;

		/** Parse `line`: its code and parameters, or nothing if it is a comment or empty */
		static std::optional<Command> parse(std::string_view line);

		/** Act on one line of G-code: append any text replied to `reply`, and other replies to `replies` */
		void executeLine(std::string_view line, std::string& reply, std::vector<Json>& replies);

		/** Act on G-code `command`; append any reply text to `reply` */
		void run(const Command& command, std::string& reply, std::vector<Json>& replies);

		/** G0/G1: move the axes named to their positions (relative after G91), at the feed rate F */
		void move(const Command& command, std::string& reply);

		/** Set heater `heater`'s active temperature `active` (or standby), if allowed; reply if not */
		void setHeaterTemperature(size_t heater, std::optional<float> active, std::optional<float> standby,
								  std::string& reply);

		/** Make `tool` the tool in use (-1 for none), its heaters active and the previous tool's on standby */
		void selectTool(int32_t tool);

		/** Start printing the job at `path` (M32, M23 then M24); reply if there is no such job */
		void startJob(std::string_view path, std::string& reply);

		/** End the job: it finished, or was cancelled */
		void endJob();

		/** Turn every heater off and stop every spindle and fan (M112, M0 after cancel) */
		void stopEverything();

		// MachineModel.cpp

		/** The reply to M409 with key `key` and flags `flags` */
		Json modelReply(std::string_view key, std::string_view flags);

		/** A number rounded to `decimals` places, so readings look like a Duet's */
		static double rounded(double value, int decimals = 2);

		// MachineFiles.cpp

		/** A thumbnail a job has, as a slicer embeds it: a QOI image, base64 encoded, at an offset in the file */
		struct Thumbnail
		{
			int32_t width = 0;	   // Pixels
			int32_t height = 0;	   // Pixels
			uint32_t offset = 0;   // Where its base64 text starts in the file
			std::string base64{};  // The image, base64 encoded
		};

		/** A heightmap: a grid of the bed's height errors over X and Y, some perhaps not probed */
		struct Mesh
		{
			float min[2] = {0.0f, 0.0f};			 // The grid's lowest X and Y
			float max[2] = {0.0f, 0.0f};			 // Its highest
			float spacing[2] = {0.0f, 0.0f};		 // Between points
			size_t points[2] = {0, 0};				 // Points along X and Y
			std::vector<std::optional<float>> z{};	 // Row by row (Y), each X along it; nothing where not probed
		};

		/** The heightmap files in 0:/sys, which G29 S1 can use */
		static const std::vector<std::string_view>& heightmapNames();

		/** Heightmap `name` (one of heightmapNames()), made for the bed: its first two axes */
		Mesh mesh(std::string_view name) const;

		/** What a heightmap's probed points come to */
		struct MeshStats
		{
			float low = 0.0f;		// The lowest error, mm
			float high = 0.0f;		// The highest
			double mean = 0.0;		// Their mean
			double deviation = 0.0; // Their standard deviation
		};

		/** What `mesh`'s probed points come to */
		static MeshStats meshStats(const Mesh& mesh);

		/** `mesh` as RepRapFirmware writes it to a file */
		static std::string meshCsv(const Mesh& mesh);

		/** move.compensation: the heightmap in use, if any */
		Json compensationModel() const;

		/** G29, G30, G32: probe the bed for a new heightmap and use it, use a saved one or stop using one, level */
		void probe(const Command& command, std::string& reply);

		/** The thumbnails `file` has: none, or a small one and a large one, as slicers embed */
		static std::vector<Thumbnail> thumbnails(const JobFile& file);

		/** A job file's size: made up from how long it takes and whether it has thumbnails, the same every time */
		static int64_t fileSize(const JobFile& file);

		/** When a job file was last changed: made up from what it holds, so it changes when the job does (the
		 * screen's cache of file information and thumbnails is kept by it), as after slicing again */
		static std::string fileDate(const JobFile& file);

		/** A job's height: 0.2 mm layers */
		static float jobHeight(const JobFile& file);

		/** The reply to M20 S3 for directory `dir`, from file `first` */
		Json fileListReply(std::string_view dir, int32_t first) const;

		/** The reply to M36 for `path` */
		Json fileInfoReply(std::string_view path) const;

		/** The reply to M36.1 for `path` from `offset`: a part of a thumbnail's base64 text */
		Json thumbnailReply(std::string_view path, uint32_t offset) const;

		/** The reply to M36.2 for `path` from `offset`: a part of the file's text */
		Json fileContentsReply(std::string_view path, uint32_t offset) const;

		/** The date every simulated file but a job has */
		static constexpr const char* FILE_DATE = "2026-11-01T12:00:00";

		/** The object model's fields that change often, those a live (f) request reports, by key */
		static const Json& liveFields();

		/** `model` with only the fields in `mask` (true to keep a whole value) */
		static Json keep(const Json& model, const Json& mask);

		/** `model` without the fields in `mask` */
		static Json drop(const Json& model, const Json& mask);

		/** Count a new seq for every key of the object model whose other (non live) fields changed */
		void updateSeqs(const Json& model);

		/** seqs: each key's seq, and the reply's */
		Json seqsModel() const;

		/** The heater's object model entry */
		Json heaterModel(size_t heater) const;

		/** The job's object model entry */
		Json jobModel() const;

		/** The job file at `path` (with or without its directory), or nullptr */
		const JobFile* findFile(std::string_view path) const;

		/** The temperature heaters cool down to, °C */
		static constexpr float AMBIENT = 21.0f;

		/** Seconds a pause, a resume, a cancel or a tool change takes */
		static constexpr double TRANSITION_TIME = 1.0;

		Config m_config;							  // What the machine is
		std::vector<HeaterState> m_heaters{};			  // By number
		std::vector<ToolState> m_tools{};				  // By number
		std::vector<AxisState> m_axes{};				  // In order
		std::vector<ExtruderState> m_extruders{};		  // By number
		std::vector<FanState> m_fans{};				  // By number
		std::vector<SpindleState> m_spindles{};		  // By number
		JobState m_job;								  // The job
		std::optional<Transition> m_transition{};		  // A change of status under way
		std::optional<MessageBox> m_messageBox{};		  // The message box showing
		int32_t m_messageBoxSeq = 0;				  // The last message box's seq
		std::string m_displayMessage{};				  // M117's message
		std::optional<std::string> m_forcedStatus{};	  // A status forced from the screen
		bool m_halted = false;						  // Emergency stopped (M112) until reset (M999)
		int32_t m_currentTool = -1;					  // Tool in use
		int32_t m_previousTool = -1;				  // Tool used before it
		bool m_relative = false;					  // Moves are relative (G91)
		bool m_relativeExtrusion = false;			  // Extrusion is relative (M83)
		float m_feedRate = 50.0f;					  // Feed rate for moves, mm/s
		float m_speedFactor = 1.0f;					  // M220, 1 = 100%
		int32_t m_workplace = 0;					  // Workplace in use, 0 = G54
		double m_upTime = 0.0;						  // Seconds since it started
		float m_noise = 0.3f;						  // Noise on temperature readings, °C
		std::mt19937 m_random{1234};				  // For noise; seeded, so runs repeat
		std::map<std::string, int32_t> m_seqs;		  // seqs, by key
		std::map<std::string, std::string> m_digests; // Each key's non live fields when its seq was last counted
		int32_t m_replySeq = 0;						  // seqs.reply
		bool m_configChanged = false;				  // G-code changed the configuration (takeConfigChange())
		std::string m_heightmap{};					  // The heightmap in use (G29 S1), its file name; "" for none
		int32_t m_probes = 0;						  // Times the bed was probed (G29), which varies heightmap.csv
		std::vector<bool> m_pushedRelative{};			  // Relative modes pushed by M120, popped by M121
	};
} // namespace Sim
