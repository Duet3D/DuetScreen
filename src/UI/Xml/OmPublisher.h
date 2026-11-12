/*
 * OmPublisher.h
 *
 * Mirrors the object model into globally named subjects and list models, so any XML view can show printer data
 * without C++. These names are the public UI API (see UI_API_VERSION in XmlUi.h): renaming or removing one breaks
 * user bundles.
 *
 * Models
 *   om.fans           one item per fan in the object model, in slot order
 *     index           position in the list
 *     number          fan number, as used by M106 P
 *     name            display name ("Fan 1")
 *     requested       requested speed, percent 0-100
 *     actual          measured speed, percent 0-100
 *     rpm             tachometer reading, -1 if none
 *     thermostatic    1 if the fan is temperature controlled, else 0
 *
 * Values are only written when they change, so a poll that changes nothing redraws nothing.
 */

#pragma once

#include "XmlListModel.h"
#include "XmlSubjects.h"

namespace UI::Xml
{
	struct FanItem : ListItemBase
	{
		using ListItemBase::ListItemBase;

		void bind(ItemBinder& b)
		{
			b("number", number)("name", name)("requested", requested)("actual", actual)("rpm", rpm)("thermostatic",
																									  thermostatic);
		}

		IntSubject number;
		StringSubject<32> name;
		IntSubject requested;
		IntSubject actual;
		IntSubject rpm{-1};
		IntSubject thermostatic;
	};

	class OmPublisher
	{
	  public:
		static void create();
		static void destroy();
		/** The live publisher, or nullptr outside XmlUi::init()/deinit() */
		static OmPublisher* get();

		ListModel<FanItem>& fans() { return m_fans; }

		/** Re-read everything (after connecting, or a language change) */
		void refresh();

		void syncFans();
		void clear();

	  private:
		OmPublisher();

		ListModel<FanItem> m_fans{"om.fans"};
	};
} // namespace UI::Xml
