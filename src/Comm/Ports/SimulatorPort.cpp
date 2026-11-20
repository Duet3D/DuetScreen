/*
 * SimulatorPort.cpp
 */

#include "SimulatorPort.h"
#include "Simulator/Simulator.h"

namespace Comm
{
	SimulatorPort::~SimulatorPort()
	{
		close();
	}

	bool SimulatorPort::open(Receiver receiver)
	{
		return Sim::Simulator::get().connect([receiver = std::move(receiver)](std::string_view reply)
											 { receiver(0, reply); });
	}

	void SimulatorPort::close()
	{
		Sim::Simulator::get().disconnect();
	}

	bool SimulatorPort::write(size_t, std::string_view bytes)
	{
		Sim::Simulator::get().send(bytes);
		return true;
	}
} // namespace Comm
