/*
 * OmWatch.h
 *
 * Calls a function whenever the object model changes at, below or above one of some paths, once the change has reached
 * the `om.*` names (see OmBindings::watch()): for C++ that reads the object model itself, a data source or a presenter.
 * It stops when it is destroyed, so a member watch can't outlive its owner.
 *
 *     OmWatch m_watch{{"tools", "heat.heaters"}, [this] { update(); }};
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace UI::Xml
{
	class OmWatch
	{
	  public:
		/** Watch nothing */
		OmWatch() = default;

		/** Call `callback` after each change to one of `paths`; nothing (with an error) if the UI isn't running */
		OmWatch(std::vector<std::string> paths, std::function<void()> callback);

		/** Stop watching */
		~OmWatch();

		OmWatch(const OmWatch&) = delete;
		OmWatch& operator=(const OmWatch&) = delete;

		/** Take over `other`'s watch, stopping this one's */
		OmWatch& operator=(OmWatch&& other) noexcept;

	  private:
		/** Stop watching, if watching */
		void stop();

		size_t m_id = 0; // The watcher's id (see OmBindings::watch()), or 0 if none
	};
} // namespace UI::Xml
