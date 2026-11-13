/*
 * NotificationsPresenter.cpp
 */

#include "NotificationsPresenter.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "version.h"
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cstdlib>
#include <regex>

namespace UI
{
	namespace
	{
		constexpr uint32_t TICK_MS = 50; // How often the countdowns are shown

		/** A control's state, as published */
		enum Control : int32_t
		{
			Hidden = 0,
			Disabled = 1,
			Usable = 2
		};

		/** The title of a response of `type` */
		std::string responseTitle(ResponseType type)
		{
			switch (type)
			{
			case ResponseType::SUCCESS:
				return _("msgbox.response_success");
			case ResponseType::WARNING:
				return _("msgbox.response_warning");
			case ResponseType::ERROR:
				return _("msgbox.response_error");
			case ResponseType::INFO:
			default:
				return _("msgbox.response_info");
			}
		}
	} // namespace

	void AlertChoiceRow::fields(Xml::FieldVisitor& f)
	{
		f.field("text", text, "The choice");
	}

	void AlertAxisRow::fields(Xml::FieldVisitor& f)
	{
		f.field("label", label, "Axis and position, e.g. \"X = 0.00\"");
		f.field("homed", homed, "1 if the axis can be jogged");
	}

	NotificationsPresenter::NotificationsPresenter()
		: XmlPresenter("notifications")
	{
		m_timer = lv_timer_create(tickCb, TICK_MS, this);
		lv_timer_pause(m_timer);

		action("response.close", [this](const Xml::ActionArgs&) { closeResponse(); });
		action("response.open_console",
			   [this](const Xml::ActionArgs&)
			   {
				   m_responses.clear();
				   m_responseOpen.set(0);
				   openScreen("console");
			   });

		action("update.confirm",
			   [this](const Xml::ActionArgs&)
			   {
				   m_updateOpen.set(0);
				   if (m_install)
				   {
					   m_install();
				   }
			   });
		action("update.close", [this](const Xml::ActionArgs&) { m_updateOpen.set(0); });

		action("alert.ok",
			   [this](const Xml::ActionArgs&)
			   {
				   const uint32_t seq = m_alert.seq;
				   switch (m_alert.mode)
				   {
				   case OM::Alert::Mode::NumberInt:
					   Xml::Services::sendGcode(fmt::format("M292 R{{{:d}}} S{:d}", std::atoi(m_input.c_str()), seq));
					   break;
				   case OM::Alert::Mode::NumberFloat:
					   Xml::Services::sendGcode(
						   fmt::format("M292 R{{{:g}}} S{:d}", static_cast<float>(std::atof(m_input.c_str())), seq));
					   break;
				   case OM::Alert::Mode::Text:
				   {
					   // Quotes are doubled inside a G-code string
					   std::string text = std::regex_replace(m_input, std::regex("\""), "\"\"");
					   text = std::regex_replace(text, std::regex("'"), "''");
					   Xml::Services::sendGcode(fmt::format("M292 R{{\"{:s}\"}} S{:d}", text, seq));
					   break;
				   }
				   default:
					   Xml::Services::sendGcode(fmt::format("M292 S{:d}", seq));
					   break;
				   }
				   closeAlert();
			   });
		action("alert.cancel",
			   [this](const Xml::ActionArgs&)
			   {
				   if (m_cancelSends)
				   {
					   Xml::Services::sendGcode(fmt::format("M292 P1 S{:d}", m_alert.seq));
				   }
				   closeAlert();
			   });
		action("alert.choose",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<AlertChoiceRow>())
				   {
					   Xml::Services::sendGcode(fmt::format("M292 R{{{:d}}} S{:d}", row->index.get(), m_alert.seq));
				   }
				   closeAlert();
			   });
		action("alert.jog",
			   [](const Xml::ActionArgs& args)
			   {
				   const auto* row = args.row<AlertAxisRow>();
				   const auto distance = args.value();
				   if (row == nullptr || !distance)
				   {
					   return;
				   }
				   Xml::Services::sendGcode(fmt::format("M120\nG91\nG1 {:c}{:g} F300\nM121", row->letter, *distance));
			   });
		action("alert.input", [this](const Xml::ActionArgs& args) { validateInput(args.text()); });
		action("alert.keyboard",
			   [this](const Xml::ActionArgs& args) { m_alertKeyboard.set(args.argInt().value_or(0) != 0 ? 1 : 0); });
		action("alert.dismissed", [this](const Xml::ActionArgs&) { closeAlert(); });
	}

	NotificationsPresenter::~NotificationsPresenter()
	{
		if (m_timer != nullptr && lv_is_initialized())
		{
			lv_timer_delete(m_timer);
		}
	}

	void NotificationsPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("response.open", m_responseOpen, "1 while a response is showing");
		f.field("response.title", m_responseTitle, "Its title");
		f.field("response.text", m_responseText, "The response");
		f.field("response.type", m_responseType, "0 info, 1 success, 2 warning, 3 error");
		f.field("response.progress", m_responseProgress, "Time left, 0-100");

		f.field("update.open", m_updateOpen, "1 while an update is offered or its result shown");
		f.field("update.title", m_updateTitle, "Title");
		f.field("update.text", m_updateText, "Text");
		f.field("update.ok_text", m_updateOkText, "OK button text");
		f.field("update.can_cancel", m_updateCanCancel, "1 if the update can be declined");

		f.field("alert.open", m_alertOpen, "1 while an M291 alert is showing");
		f.field("alert.title", m_alertTitle, "Title");
		f.field("alert.text", m_alertText, "Text");
		f.field("alert.ok", m_alertOk, "OK: 0 hidden, 1 disabled, 2 usable");
		f.field("alert.ok_text", m_alertOkText, "OK button text");
		f.field("alert.cancel", m_alertCancel, "Cancel: 0 hidden, 2 usable");
		f.field("alert.footer_shown", m_alertFooterShown, "1 if there is an OK or Cancel button");
		f.list("alert.choices", m_alertChoices, "The choices offered");
		f.field("alert.choices_shown", m_alertChoicesShown, "1 if choices are offered");
		f.list("alert.axes", m_alertAxes, "The axes the user may jog");
		f.field("alert.axes_shown", m_alertAxesShown, "1 if there are axes to jog");
		f.field("alert.input_shown", m_alertInputShown, "1 if a value is asked for");
		f.field("alert.input", m_alertInput, "The value to start with");
		f.field("alert.accepted_chars", m_alertAccepted, "Characters the value may contain; empty for any");
		f.field("alert.keyboard_mode", m_alertKeyboardMode, "\"number\" or \"text\"");
		f.field("alert.min", m_alertMin, "Lower limit; empty if none");
		f.field("alert.max", m_alertMax, "Upper limit; empty if none");
		f.field("alert.warning", m_alertWarning, "Why the value isn't valid; empty if it is");
		f.field("alert.keyboard", m_alertKeyboard, "1 while the keyboard is up");
		f.field("alert.progress", m_alertProgress, "Time left, 0-100");
		f.field("alert.progress_shown", m_alertProgressShown, "1 if the alert closes itself");
	}

	void NotificationsPresenter::onInit()
	{
		registerEventListener<EventType::Response>(this, &NotificationsPresenter::newResponse);
		registerEventListener<EventType::Alert>(this, &NotificationsPresenter::newAlertData);
		registerEventListener<EventType::UpdateAvailable>(this, &NotificationsPresenter::newUpdateAvailable);
		registerEventListener<EventType::GithubUpdateAvailable>(this,
																&NotificationsPresenter::newGithubUpdateAvailable);
		registerEventListener<EventType::UpdateResult>(this, &NotificationsPresenter::handleUpdateResult);
		registerEventListener<EventType::AxesData>([this]() { updateAlertAxes(); });
		registerEventListener<EventType::UsbDrive>(
			[this](const std::string& path, bool mounted)
			{
				// The update offered was on the drive just removed
				if (!mounted && !m_updateFile.empty() && m_updateFile.starts_with(path))
				{
					m_updateFile.clear();
					m_updateOpen.set(0);
				}
			});
	}

	void NotificationsPresenter::onActivate()
	{
		updateAlertAxes();
	}

	void NotificationsPresenter::onDisconnect()
	{
		UI_LOCK();
		m_responses.clear();
		m_responseOpen.set(0);
		m_alertAxes.clear();
	}

	void NotificationsPresenter::newResponse(const ResponseType type, const std::string& response)
	{
		ZoneScoped;
		UI_LOCK();
		// The console shows responses itself
		if (response.empty() || getCurrentScreenName() == "console" ||
			type < StorageHelper::getData(ID_NOTIFICATION_LEVEL))
		{
			return;
		}

		m_responses.push_back({type, response});
		if (m_responses.size() == 1)
		{
			showFirstResponse();
		}
	}

	void NotificationsPresenter::showFirstResponse()
	{
		if (m_responses.empty() || m_alertOpen.get() != 0)
		{
			m_responseOpen.set(0);
			return;
		}

		const Response& response = m_responses.front();
		m_responseTitle.set(responseTitle(response.type));
		m_responseText.set(response.text);
		m_responseType.set(static_cast<int32_t>(response.type));

		// Errors stay until closed, unless the user wants those to close themselves too
		const bool closes =
			response.type != ResponseType::ERROR || StorageHelper::getData(ID_NOTIFICATION_AUTO_CLOSE_ERROR);
		const auto timeout = StorageHelper::getData(ID_NOTIFICATION_TIMEOUT);
		m_responseCountdown = {.start = lv_tick_get(), .duration = closes ? static_cast<uint32_t>(timeout.count()) : 0};
		m_responseProgress.set(remaining(m_responseCountdown));
		m_responseOpen.set(1);
		lv_timer_resume(m_timer);
	}

	void NotificationsPresenter::closeResponse()
	{
		if (!m_responses.empty())
		{
			m_responses.pop_front();
		}
		m_responseCountdown = {};
		m_responseOpen.set(0);
		showFirstResponse();
	}

	void NotificationsPresenter::newAlertData(const OM::Alert& alert)
	{
		ZoneScoped;
		UI_LOCK();
		if (alert.mode == OM::Alert::Mode::None)
		{
			if (m_alertOpen.get() != 0)
			{
				closeAlert();
			}
			return;
		}

		using Mode = OM::Alert::Mode;
		m_alert = alert;
		m_alertTitle.set(alert.title.c_str());
		m_alertText.set(alert.text.c_str());

		// Buttons
		const bool input = alert.mode == Mode::NumberInt || alert.mode == Mode::NumberFloat || alert.mode == Mode::Text;
		// Every message can be acknowledged, even one the printer sends without buttons (Info)
		const bool ok = alert.mode == Mode::Info || alert.mode == Mode::InfoClose || alert.mode == Mode::InfoConfirm ||
						alert.mode == Mode::ConfirmCancel || input;
		m_alertOkText.set(alert.mode == Mode::InfoClose ? _("common.close") : _("common.ok"));
		m_alertOk.set(ok ? Usable : Hidden);
		m_cancelSends = alert.cancelButton;
		m_alertCancel.set(alert.mode == Mode::ConfirmCancel || alert.cancelButton ? Usable : Hidden);
		m_alertFooterShown.set(m_alertOk.get() != Hidden || m_alertCancel.get() != Hidden ? 1 : 0);

		// Choices
		m_alertChoices.sync(alert.mode == Mode::Choices ? alert.choices_count : 0,
							[&](AlertChoiceRow& row, size_t i) { row.text.set(alert.choices[i].c_str()); });
		m_alertChoicesShown.set(alert.mode == Mode::Choices ? 1 : 0);

		// A value to type
		m_alertInputShown.set(input ? 1 : 0);
		m_alertWarning.set("");
		m_alertKeyboard.set(0);
		m_alertMin.set("");
		m_alertMax.set("");
		std::string value;
		switch (alert.mode)
		{
		case Mode::NumberInt:
			m_alertAccepted.set("+-0123456789");
			m_alertKeyboardMode.set("number");
			if (alert.limits.numberInt.min > INT32_MIN)
			{
				m_alertMin.set(fmt::format("{:d}", alert.limits.numberInt.min));
			}
			if (alert.limits.numberInt.max < INT32_MAX)
			{
				m_alertMax.set(fmt::format("{:d}", alert.limits.numberInt.max));
			}
			value = fmt::format("{:d}", alert.limits.numberInt.valueDefault);
			break;
		case Mode::NumberFloat:
			m_alertAccepted.set("+-0123456789.");
			m_alertKeyboardMode.set("number");
			if (alert.limits.numberFloat.min > -FLT_MAX)
			{
				m_alertMin.set(fmt::format("{:g}", alert.limits.numberFloat.min));
			}
			if (alert.limits.numberFloat.max < FLT_MAX)
			{
				m_alertMax.set(fmt::format("{:g}", alert.limits.numberFloat.max));
			}
			value = fmt::format("{:.1f}", alert.limits.numberFloat.valueDefault);
			break;
		case Mode::Text:
			m_alertAccepted.set("");
			m_alertKeyboardMode.set("text");
			value = alert.limits.text.valueDefault.c_str();
			break;
		default:
			break;
		}
		m_alertInput.set(value);
		if (input)
		{
			validateInput(value);
		}

		// Axes to jog
		std::vector<char> letters;
		if (alert.flags.IsBitSet(OM::Alert::GotControls))
		{
			for (size_t i = 0; i < MAX_TOTAL_AXES; ++i)
			{
				auto axis = (alert.controls & (1u << i)) ? OM::Move::GetAxis(i) : nullptr;
				if (axis != nullptr)
				{
					letters.push_back(axis->letter[0]);
				}
			}
		}
		m_alertAxes.sync(letters.size(), [&](AlertAxisRow& row, size_t i) { row.letter = letters[i]; });
		m_alertAxesShown.set(letters.empty() ? 0 : 1);
		updateAlertAxes();

		// A countdown, if it closes itself
		m_alertCountdown = {.start = lv_tick_get(), .duration = static_cast<uint32_t>(alert.timeout.count())};
		m_alertProgress.set(remaining(m_alertCountdown));
		m_alertProgressShown.set(m_alertCountdown.duration > 0 ? 1 : 0);
		if (m_alertCountdown.duration > 0)
		{
			lv_timer_resume(m_timer);
		}

		// Responses wait until the alert is answered
		m_responseOpen.set(0);
		m_alertOpen.set(1);
	}

	void NotificationsPresenter::closeAlert()
	{
		m_alertCountdown = {};
		m_alertKeyboard.set(0);
		m_alertOpen.set(0);
		showFirstResponse();
	}

	void NotificationsPresenter::validateInput(std::string_view text)
	{
		m_input = text;
		using Mode = OM::Alert::Mode;
		bool valid = true;
		std::string warning;
		switch (m_alert.mode)
		{
		case Mode::NumberInt:
		{
			const auto& limits = m_alert.limits.numberInt;
			const int value = std::atoi(m_input.c_str());
			valid = value >= limits.min && value <= limits.max;
			warning = _("msgbox.warning_int_range", limits.min, limits.max);
			break;
		}
		case Mode::NumberFloat:
		{
			const auto& limits = m_alert.limits.numberFloat;
			const auto value = static_cast<float>(std::atof(m_input.c_str()));
			valid = value >= limits.min && value <= limits.max;
			warning = _("msgbox.warning_float_range", limits.min, limits.max);
			break;
		}
		case Mode::Text:
		{
			const auto& limits = m_alert.limits.text;
			const auto length = static_cast<int32_t>(m_input.length());
			valid = length >= limits.min && length <= limits.max;
			warning = _("msgbox.warning_text_length", limits.min, limits.max);
			break;
		}
		default:
			break;
		}
		m_alertWarning.set(valid ? std::string() : warning);
		m_alertOk.set(valid ? Usable : Disabled);
	}

	void NotificationsPresenter::updateAlertAxes()
	{
		for (size_t i = 0; i < m_alertAxes.size(); i++)
		{
			AlertAxisRow& row = m_alertAxes[i];
			if (auto axis = OM::Move::GetAxisByLetter(row.letter))
			{
				row.label.set(fmt::format("{:c} = {:.2f}", row.letter, axis->userPosition));
				row.homed.set(axis->homed ? 1 : 0);
			}
		}
	}

	void NotificationsPresenter::offerUpdate(std::string text, std::function<void()> install)
	{
		m_updateTitle.set(_("message.update_available"));
		m_updateText.set(text);
		m_updateOkText.set(_("message.update_confirm"));
		m_updateCanCancel.set(1);
		m_install = std::move(install);
		m_updateOpen.set(1);
	}

	void NotificationsPresenter::newUpdateAvailable(const std::string& file)
	{
		m_updateFile = file;
		offerUpdate(_("message.update_available_text"), [this]() { UpgradeHelper::upgradeFromUSB(m_updateFile); });
	}

	void NotificationsPresenter::newGithubUpdateAvailable(const std::string& latestVersion)
	{
		offerUpdate(_("message.update_available_github_text", latestVersion, FIRMWARE_VERSION),
					[]() { UpgradeHelper::upgradeFromGithubLatest(); });
	}

	void NotificationsPresenter::handleUpdateResult(const UpgradeHelper::UpgradeInfo& info)
	{
		ZoneScoped;
		switch (info.result)
		{
		case UpgradeHelper::UpgradeResult::Success:
			m_updateTitle.set(_("message.update_successful"));
			m_updateText.set(_("message.update_successful_text", info.currentVersion, info.currentBuildrootVersion));
			break;
		case UpgradeHelper::UpgradeResult::BuildrootVersionError:
			m_updateTitle.set(_("message.update_failed"));
			m_updateText.set(
				_("message.update_buildroot_error_text", info.currentBuildrootVersion, info.updateBuildrootVersion));
			break;
		case UpgradeHelper::UpgradeResult::BuildrootVersionWarning:
			m_updateTitle.set(_("message.update_buildroot_warning"));
			m_updateText.set(_("message.update_buildroot_warning_text",
							   info.currentVersion,
							   info.currentBuildrootVersion,
							   info.updateBuildrootVersion));
			break;
		}
		m_updateOkText.set(_("common.ok"));
		m_updateCanCancel.set(0);
		m_install = nullptr;
		m_updateOpen.set(1);
	}

	void NotificationsPresenter::tickCb(lv_timer_t* timer)
	{
		UI_LOCK();
		auto& self = *static_cast<NotificationsPresenter*>(lv_timer_get_user_data(timer));

		if (self.m_responseOpen.get() != 0 && self.m_responseCountdown.duration > 0)
		{
			self.m_responseProgress.set(remaining(self.m_responseCountdown));
			if (self.m_responseProgress.get() == 0)
			{
				self.closeResponse();
			}
		}
		if (self.m_alertOpen.get() != 0 && self.m_alertCountdown.duration > 0)
		{
			self.m_alertProgress.set(remaining(self.m_alertCountdown));
			if (self.m_alertProgress.get() == 0)
			{
				// As if cancelled
				if (self.m_cancelSends)
				{
					Xml::Services::sendGcode(fmt::format("M292 P1 S{:d}", self.m_alert.seq));
				}
				self.closeAlert();
			}
		}

		// Nothing counting down: rest until something is
		const bool responseCounting = self.m_responseOpen.get() != 0 && self.m_responseCountdown.duration > 0;
		const bool alertCounting = self.m_alertOpen.get() != 0 && self.m_alertCountdown.duration > 0;
		if (!responseCounting && !alertCounting)
		{
			lv_timer_pause(timer);
		}
	}

	int32_t NotificationsPresenter::remaining(const Countdown& countdown)
	{
		if (countdown.duration == 0)
		{
			return 0;
		}
		const uint32_t elapsed = lv_tick_elaps(countdown.start);
		return elapsed >= countdown.duration
				   ? 0
				   : static_cast<int32_t>(100 * (countdown.duration - elapsed) / countdown.duration);
	}
} // namespace UI
