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
 * Each is a watcher on the object model (Tree::watch()), so it runs as a response is applied, and the keys it watches
 * are kept up to date whatever the UI shows (Tree::want()); so is `directories`, which file paths are made from.
 */

#pragma once

namespace OM::Followers
{
	/** Start following the object model; calling it again has no effect */
	void start();

	/**
	 * Act on the object model as it is now, as if all of it had changed: on connecting, when the status the
	 * connection set ("connecting") must give way to the printer's even if the model hasn't changed
	 */
	void refresh();
} // namespace OM::Followers
