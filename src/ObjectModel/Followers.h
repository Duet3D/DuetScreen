/*
 * Followers.h
 *
 * What DuetScreen itself does as the printer's object model changes, besides showing it (which the UI does from the
 * tree itself):
 *
 *   - state.status          the printer's status (PrinterStatus.h), for the connection and the UI's state
 *   - boards[0].uniqueId    which printer this is: a different one has different files and settings
 *   - state.time            the screen's clock follows the printer's, every TIME_SYNC_INTERVAL at most
 *   - state.thisInput       the input channel the screen's G-code goes to, for its distance unit
 *
 * Each is a watcher on the object model (Tree::watch()), so it runs as a response is applied.
 */

#pragma once

namespace OM::Followers
{
	/** Start following the object model; calling it again has no effect */
	void start();
} // namespace OM::Followers
