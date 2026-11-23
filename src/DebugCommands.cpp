#include "Debug.h"

#include "Comm/Connection.h"
#include "DebugCommands.h"
#include "Hardware/Usb.h"
#include <map>

namespace Debug
{
	// Use construct-on-first-use to avoid static initialization order issues
	static std::map<const char*, DebugCommand*>& registry()
	{
		static std::map<const char*, DebugCommand*> commandsMap;
		return commandsMap;
	}

// TODO reenable log files
#if 0
	static DebugCommand s_verboseLog("dbg_verbose_log",
									 []()
									 {
										 // Create a log file and send it to the Duet
										 system("logcat -v threadtime -d *:V > /tmp/DuetScreen_log.txt");
										 std::string logs;
										 //  USB::ReadFileContents("/tmp/DuetScreen_log.txt", logs);
										 Comm::Connection::get().writeFile("/sys/DuetScreen_log.txt", logs);
										 system("rm /tmp/DuetScreen_log.txt");
									 });

	static DebugCommand s_errorLog("dbg_error_log",
								   []()
								   {
									   // Create a log file and send it to the Duet
									   system("logcat -v threadtime -d *:W > /tmp/DuetScreen_log.txt");
									   std::string logs;
									   USB::ReadFileContents("/tmp/DuetScreen_log.txt", logs);
									   Comm::Connection::get().writeFile("/sys/DuetScreen_error_log.txt", logs);
									   system("rm /tmp/DuetScreen_log.txt");
								   });
#endif

	void CreateCommand(const char* id, debugCb_t callback)
	{
		new DebugCommand(id, callback);
	}

	DebugCommand::DebugCommand(const char* id, debugCb_t callback)
		: m_id(id)
		, m_callback(callback)
	{
		registry()[id] = this;
	}

	DebugCommand* GetCommand(const char* id)
	{
		auto& cmds = registry();
		auto it = cmds.find(id);
		if (it == cmds.end())
		{
			LOG_WARN("id {:s} not found", id);
			return nullptr;
		}
		return it->second;
	}

	DebugCommand* GetCommandByIndex(size_t index)
	{
		auto& cmds = registry();
		if (index >= cmds.size())
		{
			LOG_WARN("Index out of range");
			return nullptr;
		}
		auto it = cmds.begin();
		for (size_t i = 0; i < index; i++)
		{
			it++;
		}
		return it->second;
	}

	size_t GetCommandCount()
	{
		return registry().size();
	}
} // namespace Debug
