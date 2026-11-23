/*
 * ScriptedPrinter.h
 *
 * A printer a test plays: a SerialPort the connection talks to (Comm::Connection::connectThrough()) that keeps the
 * lines the screen sends, and answers one when the test says, with the reply the test gives, numbered as
 * RepRapFirmware numbers it so the screen gives it to the request it answers.
 */

#pragma once

#include "Comm/Ports/SerialPort.h"
#include <set>
#include <string>
#include <string_view>
#include <vector>

class ScriptedPrinter : public Comm::SerialPort
{
  public:
	bool open(Receiver receiver) override;
	void close() override;
	bool write(size_t channel, std::string_view bytes) override;
	size_t channels() const override { return 1; }
	bool needsChecksums() const override { return false; }

	/**
	 * Answer the oldest line sent, and not yet answered, whose command starts with `command` (e.g. "M20"); the
	 * connection is run before (so what the screen asked is sent) and after (so the answer is taken)
	 * @param reply The reply, a JSON object; its "line" is added
	 * @return whether there was such a line
	 */
	bool answer(std::string_view command, std::string_view reply);

	/** Every line the screen has sent, without its line number, oldest first */
	std::vector<std::string> sent() const;

  private:
	Receiver m_receiver;			  // Where replies go
	std::vector<std::string> m_lines; // The lines sent, as sent
	std::set<size_t> m_answered;	  // Indices in m_lines of those answered
};
