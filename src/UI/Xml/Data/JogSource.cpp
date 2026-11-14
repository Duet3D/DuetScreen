/*
 * JogSource.cpp
 */

#include "JogSource.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <algorithm>
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		size_t s_distance = 4; // Index of the chosen distance; clamped to the distances there are
		size_t s_feedrate = 3; // Index of the chosen speed; clamped to the speeds there are

		/** `index` clamped to a list of `count`; 0 for an empty one */
		size_t clamped(size_t index, size_t count)
		{
			return count == 0 ? 0 : std::min(index, count - 1);
		}

		/** Tell every view the choice changed */
		void changed()
		{
			Model::get().post<EventType::JogChoice>();
		}
	} // namespace

	void JogChoiceRow::fields(FieldVisitor& f)
	{
		f.field("text", text, "The value, e.g. \"0.1\"");
		f.field("selected", selected, "1 for the one in use");
	}

	JogSource::JogSource()
		: DataSource("jog")
	{
	}

	void JogSource::fields(FieldVisitor& f)
	{
		f.field("distance_title", m_distanceTitle, "The distances' title, with the unit, e.g. \"Move Distance (mm)\"");
		f.field("feedrate_title", m_feedrateTitle, "The speeds' title, with the unit");
		f.list("distances",
			   m_distances,
			   "The jog distances to choose from (actions.jog.distance, actions.jog.edit_distance)");
		f.list("feedrates",
			   m_feedrates,
			   "The jog speeds to choose from (actions.jog.feedrate, actions.jog.edit_feedrate)");
	}

	void JogSource::onInit()
	{
		registerEventListener<EventType::JogChoice>(this, &JogSource::update);
	}

	void JogSource::update()
	{
		ZoneScoped;
		m_distanceTitle.set(_("move.distance", Units::getDisplayedDistanceUnit()));
		m_feedrateTitle.set(_("move.feedrate", Units::getDisplayedSpeedUnit()));
		const auto distances = StorageHelper::getData(ID_MOVE_DISTANCES);
		const auto feedrates = StorageHelper::getData(ID_MOVE_FEEDRATES);
		const size_t distance = clamped(s_distance, distances.size());
		const size_t feedrate = clamped(s_feedrate, feedrates.size());
		m_distances.sync(distances.size(),
						 [&](JogChoiceRow& row, size_t i)
						 {
							 row.text.set(fmt::format("{}", distances[i]));
							 row.selected.set(i == distance ? 1 : 0);
						 });
		m_feedrates.sync(feedrates.size(),
						 [&](JogChoiceRow& row, size_t i)
						 {
							 row.text.set(fmt::format("{}", feedrates[i]));
							 row.selected.set(i == feedrate ? 1 : 0);
						 });
	}

	std::optional<float> JogSource::distance()
	{
		const auto distances = StorageHelper::getData(ID_MOVE_DISTANCES);
		return distances.empty() ? std::nullopt : std::optional(distances[clamped(s_distance, distances.size())]);
	}

	std::optional<uint32_t> JogSource::feedrate()
	{
		const auto feedrates = StorageHelper::getData(ID_MOVE_FEEDRATES);
		return feedrates.empty() ? std::nullopt : std::optional(feedrates[clamped(s_feedrate, feedrates.size())]);
	}

	void JogSource::chooseDistance(size_t index)
	{
		s_distance = index;
		changed();
	}

	void JogSource::chooseFeedrate(size_t index)
	{
		s_feedrate = index;
		changed();
	}

	void JogSource::setDistance(size_t index, float value)
	{
		auto distances = StorageHelper::getData(ID_MOVE_DISTANCES);
		if (index < distances.size())
		{
			distances[index] = value;
			StorageHelper::setData(ID_MOVE_DISTANCES, distances);
			changed();
		}
	}

	void JogSource::setFeedrate(size_t index, uint32_t value)
	{
		auto feedrates = StorageHelper::getData(ID_MOVE_FEEDRATES);
		if (index < feedrates.size())
		{
			feedrates[index] = value;
			StorageHelper::setData(ID_MOVE_FEEDRATES, feedrates);
			changed();
		}
	}
} // namespace UI::Xml
