/*
 * NumberEntry.cpp
 */

#include "NumberEntry.h"
#include "Debug.h"
#include "utils/utils.h"
#include <algorithm>
#include <cstdlib>
#include <limits>

namespace UI::Xml
{
	namespace
	{
		constexpr std::string_view UNSIGNED_CHARS = "0123456789."; // What a box for a number of 0 or more takes
		constexpr std::string_view SIGNED_CHARS = "0123456789.-";  // What a box for any number takes

		/** `text` as a number, as typed: empty is 0, and it is kept within what a float holds */
		float parse(std::string_view text)
		{
			const double value = std::strtod(std::string(text).c_str(), nullptr);
			return static_cast<float>(std::clamp(value,
												 static_cast<double>(std::numeric_limits<float>::lowest()),
												 static_cast<double>(std::numeric_limits<float>::max())));
		}
	} // namespace

	NumberEntry::NumberEntry(std::string id)
		: m_id(std::move(id))
		, m_accepted(UNSIGNED_CHARS)
	{
	}

	NumberEntry::~NumberEntry()
	{
		watchAsker(nullptr);
	}

	void NumberEntry::fields(FieldVisitor& f)
	{
		f.field(m_id + ".header", m_header, "What is asked for, e.g. \"T0 heater 1 active\"; empty for nothing");
		f.field(m_id + ".text", m_typed, "What has been typed");
		f.field(m_id + ".accepted", m_accepted, "The characters the box takes, for `bind_accepted_chars`");
		f.field(m_id + ".confirmable",
				m_confirmable,
				"1 if what has been typed can be confirmed: a number within the limits, or any text");
		f.field(m_id + ".signed", m_signed, "1 if the number may be negative, so the +/- key works");
	}

	void NumberEntry::ask(NumberRequest request)
	{
		m_text.reset();
		watchAsker(request.asker);
		m_header.set(request.header);
		m_signed.set(request.min < 0.0f ? 1 : 0);
		m_accepted.set(request.min < 0.0f ? SIGNED_CHARS : UNSIGNED_CHARS);
		const std::optional<float> start = request.value;
		m_number = std::move(request);
		setText(start ? utils::formatFloat(*start) : std::string());
	}

	void NumberEntry::ask(TextRequest request)
	{
		m_number.reset();
		watchAsker(nullptr);
		m_header.set(request.header);
		m_signed.set(0);
		m_accepted.set(UNSIGNED_CHARS);
		const std::string start = request.text;
		m_text = std::move(request);
		setText(start);
	}

	void NumberEntry::showValue(float value)
	{
		setText(utils::formatFloat(value));
	}

	void NumberEntry::drop()
	{
		m_number.reset();
		m_text.reset();
		watchAsker(nullptr);
		setText("");
	}

	void NumberEntry::setText(std::string_view text)
	{
		m_typed.set(text);
		m_confirmable.set(confirmable() ? 1 : 0);
	}

	bool NumberEntry::confirmable() const
	{
		if (m_number)
		{
			const float value = parse(m_typed.get());
			return value >= m_number->min && value <= m_number->max;
		}
		return m_text.has_value();
	}

	void NumberEntry::confirm()
	{
		if (!confirmable())
		{
			return;
		}
		// Taken before the dialog closes and forgets them; the callback may then ask again, e.g. for the next value
		const auto onNumber = m_number ? m_number->onConfirm : nullptr;
		const auto onText = m_text ? m_text->onConfirm : nullptr;
		const std::string typed(m_typed.get());
		if (m_onConfirmed)
		{
			m_onConfirmed();
		}
		if (onNumber)
		{
			onNumber(parse(typed));
		}
		else if (onText)
		{
			onText(typed);
		}
	}

	void NumberEntry::watchAsker(lv_obj_t* asker)
	{
		if (m_asker != nullptr)
		{
			lv_obj_remove_event_cb_with_user_data(m_asker, askerDeletedCb, this);
		}
		m_asker = asker;
		if (m_asker != nullptr)
		{
			lv_obj_add_event_cb(m_asker, askerDeletedCb, LV_EVENT_DELETE, this);
		}
	}

	void NumberEntry::askerDeletedCb(lv_event_t* e)
	{
		auto* entry = static_cast<NumberEntry*>(lv_event_get_user_data(e));
		entry->m_asker = nullptr; // Going: nothing to unregister from
		entry->drop();
		if (entry->m_onDropped)
		{
			entry->m_onDropped();
		}
	}

	std::vector<NumberEntry::Action> NumberEntry::actions(const std::vector<NumberEntry*>& entries)
	{
		const std::vector<NumberEntry*> all(entries);

		// The entry an action is about: the one its arg names
		auto entryOf = [all](const ActionArgs& args) -> NumberEntry*
		{
			const auto it =
				std::find_if(all.begin(), all.end(), [&args](const NumberEntry* e) { return e->m_id == args.arg; });
			if (it == all.end())
			{
				LOG_WARN("{:s}: no number entry called '{:s}'", args.name, args.arg);
				return nullptr;
			}
			return *it;
		};

		return {
			{"numberpad.typed",
			 "What has been typed in the box of the entry named by `arg` (its id) changed. From the box's "
			 "`value_changed`.",
			 [entryOf](const ActionArgs& args)
			 {
				 if (auto* e = entryOf(args))
				 {
					 e->setText(args.text());
				 }
			 }},
			{"numberpad.clear",
			 "Empty the box of the entry named by `arg` (its id)",
			 [entryOf](const ActionArgs& args)
			 {
				 if (auto* e = entryOf(args))
				 {
					 e->setText("");
				 }
			 }},
			{"numberpad.confirm",
			 "Confirm what has been typed in the entry named by `arg` (its id), if it can be. From the keys' `ready` "
			 "(their OK key).",
			 [entryOf](const ActionArgs& args)
			 {
				 if (auto* e = entryOf(args))
				 {
					 e->confirm();
				 }
			 }},
		};
	}
} // namespace UI::Xml
