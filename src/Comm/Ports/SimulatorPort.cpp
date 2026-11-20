/*
 * SimulatorPort.cpp
 */

#include "SimulatorPort.h"
#include "Simulator/Simulator.h"
#include "tracy/Tracy.hpp"

namespace Comm
{
	SimulatorPort::~SimulatorPort()
	{
		ZoneScoped;
		close();
	}

	bool SimulatorPort::open(Receiver receiver)
	{
		ZoneScoped;
		return Sim::Simulator::get().connect([receiver = std::move(receiver)](std::string_view reply)
											 { receiver(0, reply); });
	}

	void SimulatorPort::close()
	{
		ZoneScoped;
		Sim::Simulator::get().disconnect();
	}

	bool SimulatorPort::write(size_t, std::string_view bytes)
	{
		ZoneScoped;
		Sim::Simulator::get().send(bytes);
		return true;
	}
} // namespace Comm
