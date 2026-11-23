/*
 * SimulatorPort.h
 *
 * The simulated Duet (src/Simulator), as a SerialPort: it takes G-code lines and answers as a Duet on USB does.
 * Only in builds with DUET_SIMULATOR.
 */

#pragma once

#include "SerialPort.h"

namespace Comm
{
	class SimulatorPort : public SerialPort
	{
	  public:
		/** Disconnect from the simulated Duet if connected */
		~SimulatorPort() override;

		bool open(Receiver receiver) override;
		void close() override;
		bool write(size_t channel, std::string_view bytes) override;
		size_t channels() const override { return 1; }
		bool needsChecksums() const override { return false; }
	};
} // namespace Comm
