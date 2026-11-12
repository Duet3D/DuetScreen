/*
 * OmPublisher.cpp
 */

#include "OmPublisher.h"
#include "Debug.h"
#include "ObjectModel/Fan.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <cmath>
#include <memory>

namespace UI::Xml
{
	static std::unique_ptr<OmPublisher> s_instance;

	static int32_t toPercent(float fraction)
	{
		return static_cast<int32_t>(std::lround(100.0f * fraction));
	}

	OmPublisher::OmPublisher()
	{
		// Model listeners can't be removed and the Model outlives XmlUi::init()/deinit() cycles (e.g. one per test),
		// so register once and forward to whichever publisher is live.
		static bool s_listening = false;
		if (!s_listening)
		{
			s_listening = true;
			Model& model = Model::get();
			model.addEventListener<EventType::FanData>(
				[]
				{
					if (s_instance)
						s_instance->syncFans();
				});
			model.addEventListener<EventType::Connected>(
				[]
				{
					if (s_instance)
						s_instance->refresh();
				});
			model.addEventListener<EventType::Disconnected>(
				[]
				{
					if (s_instance)
						s_instance->clear();
				});
		}
	}

	void OmPublisher::create()
	{
		s_instance.reset(new OmPublisher());
		s_instance->refresh();
	}

	void OmPublisher::destroy()
	{
		s_instance.reset();
	}

	OmPublisher* OmPublisher::get()
	{
		return s_instance.get();
	}

	void OmPublisher::refresh()
	{
		syncFans();
	}

	static void fill(FanItem& item, const OM::Fan& fan)
	{
		item.number.set(static_cast<int32_t>(fan.index));
		item.name.set(_("fine_tune.fan", fan.index));
		item.requested.set(toPercent(fan.requestedValue));
		item.actual.set(toPercent(fan.actualValue));
		item.rpm.set(fan.rpm);
		item.thermostatic.set(fan.thermostatic ? 1 : 0);
	}

	void OmPublisher::syncFans()
	{
		ZoneScoped;
		UI_LOCK();
		m_fans.resize(OM::GetFanCount(),
					  [](FanItem& item, size_t slot)
					  {
						  if (auto fan = OM::GetFanBySlot(slot))
						  {
							  fill(item, *fan);
						  }
					  });

		OM::IterateFansWhile(
			[this](OM::FanPtr fan, size_t slot)
			{
				if (slot >= m_fans.size())
				{
					return false;
				}
				fill(m_fans[slot], *fan);
				return true;
			});
	}

	void OmPublisher::clear()
	{
		UI_LOCK();
		m_fans.resize(0);
	}
} // namespace UI::Xml
