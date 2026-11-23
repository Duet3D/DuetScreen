/*
 * UsbSource.cpp
 */

#include "UsbSource.h"
#include "Comm/Connection.h"
#include "Comm/FileContents.h"
#include "Debug.h"
#include "FilesSource.h"
#include "Hardware/Usb.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"
#include <algorithm>
#include <cctype>
#include <thread>

namespace UI::Xml
{
	namespace
	{
		/** `text` in lower case */
		std::string lowered(std::string_view text)
		{
			std::string lower(text);
			std::transform(lower.begin(),
						   lower.end(),
						   lower.begin(),
						   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return lower;
		}

		/** The name `destination` is shown by, e.g. "Jobs" */
		std::string destinationName(UsbSource::Destination destination)
		{
			switch (destination)
			{
			case UsbSource::Destination::Macros:
				return _("file.macros");
			case UsbSource::Destination::System:
				return _("usb.system");
			case UsbSource::Destination::Firmware:
				return _("usb.firmware");
			case UsbSource::Destination::Jobs:
				break;
			}
			return _("file.jobs");
		}

		/** `date` as shown, to the minute: "2025-03-18T18:21:58" becomes "2025-03-18 18:21" */
		std::string shownDate(std::string date)
		{
			std::replace(date.begin(), date.end(), 'T', ' ');
			return date.substr(0, 16);
		}

		/** Set `subject` to `control` */
		void show(IntSubject& subject, FilesSource::Control control)
		{
			subject.set(static_cast<int32_t>(control));
		}
	} // namespace

	UsbSource::UsbSource()
		: DataSource("usb")
	{
	}

	void UsbSource::fields(FieldVisitor& f)
	{
		f.field("drives", m_drives, "How many USB drives are plugged into the screen");
		f.field(
			"dialog",
			m_dialog,
			"What the USB file dialog shows: \"file\" (what to do with it), \"upload\" (its upload going, or how it "
			"went); empty when it is closed");
		f.field("dialog_title", m_dialogTitle, "The dialog's title: the file's name");
		f.field("file_size", m_fileSize, "The file's size, e.g. \"1.2 MB\"");
		f.field("file_date", m_fileDate, "When the file was last changed, e.g. \"2025-03-18 18:21\"");
		f.field("destination",
				m_destination,
				"The printer's folder it is uploaded to: 0 jobs, 1 macros, 2 system, 3 firmware; its kind picks one at "
				"first (actions.usb.destination)");
		f.field("upload", m_upload, "Uploading it (actions.usb.upload): 1 disabled (see `upload_reason`), 2 usable");
		f.field("upload_reason",
				m_uploadReason,
				"Why it can't be uploaded now, e.g. \"The printer isn't connected\"; empty if it can");
		f.field(
			"upload_state",
			m_uploadState,
			"How its upload is going: 0 reading it, 1 sending it (source.files.transfer_progress), 2 done, 3 failed, "
			"4 cancelling, 5 cancelled");
		f.field(
			"cancellable",
			m_cancellable,
			"1 while the upload from a USB drive going can be cancelled (actions.usb.cancel_upload): while it is "
			"read, and while it is sent over the network; over USB and UART the printer takes the whole file once it "
			"has started. Else 0");
		f.field("upload_text",
				m_uploadText,
				"What its upload is doing, or how it went, e.g. \"benchy.gcode is now in Jobs\"");
		f.field("update",
				m_update,
				"Updating the screen with it (actions.usb.install): 0 hidden (not a *.tar.gz), 1 disabled (being "
				"checked, or not installable: see `update_text`), 2 usable");
		f.field("update_text", m_updateText, "What checking it as an update found; empty when `update` is hidden");
		f.field("installing",
				m_installing,
				"1 while an update is installed: a dialog holds the screen until it restarts, or says why it couldn't");
		f.field("install_text", m_installText, "What installing the update is doing, or why it couldn't");
		f.field("install_failed", m_installFailed, "1 if the update couldn't be installed, else 0");
	}

	void UsbSource::onInit()
	{
		registerEventListener<EventType::UsbDrive>([this](const std::string&, bool) { driveChanged(); });
		// The printer's folders, where files go, can arrive after the dialog opens
		watchObjectModel({"directories"});
		update();
	}

	void UsbSource::update()
	{
		m_drives.set(static_cast<int32_t>(USB::UsbMonitor::getInstance().getMountedDrives().size()));
		showAllowed();
	}

	void UsbSource::clear()
	{
		showAllowed();
	}

	std::string UsbSource::folderOf(Destination destination)
	{
		using Type = OM::Directories::DirectoryType;
		switch (destination)
		{
		case Destination::Macros:
			return OM::Directories::GetDirectory(Type::MACROS);
		case Destination::System:
			return OM::Directories::GetDirectory(Type::SYSTEM);
		case Destination::Firmware:
			return OM::Directories::GetDirectory(Type::FIRMWARE);
		case Destination::Jobs:
			break;
		}
		return OM::Directories::GetDirectory(Type::GCODES);
	}

	UsbSource::Destination UsbSource::destinationFor(std::string_view name)
	{
		const std::string lower = lowered(name);
		if (lower.ends_with(".g"))
		{
			return Destination::Macros;
		}
		if (lower.ends_with(".bin") || lower.ends_with(".uf2"))
		{
			return Destination::Firmware;
		}
		if (lower.ends_with(".json") || lower.ends_with(".csv"))
		{
			return Destination::System;
		}
		return Destination::Jobs;
	}

	void UsbSource::ask(const OM::FileSystem::FileSystemItem& item)
	{
		ZoneScoped;
		m_path = item.GetPath();
		m_size = item.GetSize();
		const uint64_t generation = ++m_generation;
		m_dialogTitle.set(item.GetName());
		m_fileSize.set(item.GetReadableSize());
		m_fileDate.set(shownDate(item.GetDate()));
		m_destination.set(static_cast<int32_t>(destinationFor(item.GetName())));

		m_updateInstallable = false;
		if (UpgradeHelper::looksLikeUpdate(item.GetName()))
		{
			show(m_update, FilesSource::Control::Disabled);
			m_updateText.set(_("usb.checking"));
			// The archive is decompressed all through to check it
			std::weak_ptr<DataSource> self = weak_from_this();
			std::thread(
				[self, generation, path = m_path]
				{
					tracy::SetThreadName("Update check");
					const UpgradeHelper::UpdateCheck check = UpgradeHelper::checkUpdate(path);
					if (auto source = self.lock())
					{
						static_cast<UsbSource&>(*source).updateChecked(generation, check);
					}
				})
				.detach();
		}
		else
		{
			show(m_update, FilesSource::Control::Hidden);
			m_updateText.set("");
		}
		showAllowed();
		m_dialog.set("file");
	}

	void UsbSource::updateChecked(uint64_t generation, const UpgradeHelper::UpdateCheck& check)
	{
		ZoneScoped;
		UI_LOCK();
		if (generation != m_generation)
		{
			return; // the dialog is about another file now
		}
		using Verdict = UpgradeHelper::UpdateCheck::Verdict;
		m_updateInstallable = check.installable();
		show(m_update, m_updateInstallable ? FilesSource::Control::Usable : FilesSource::Control::Disabled);
		switch (check.verdict)
		{
		case Verdict::Installable:
			m_updateText.set(_("usb.update_ok"));
			break;
		case Verdict::PatchDiffers:
			m_updateText.set(_("usb.update_patch", check.updateBuildrootVersion, check.installedBuildrootVersion));
			break;
		case Verdict::WrongSystem:
			m_updateText.set(
				_("usb.update_wrong_system", check.updateBuildrootVersion, check.installedBuildrootVersion));
			break;
		case Verdict::NotAnUpdate:
			m_updateText.set(_("usb.not_update"));
			break;
		}
	}

	void UsbSource::setDestination(Destination destination)
	{
		m_destination.set(static_cast<int32_t>(destination));
		showAllowed();
	}

	void UsbSource::showAllowed()
	{
		std::string reason;
		if (m_uploading)
		{
			reason = _("usb.another_upload");
		}
		else if (!Comm::Connection::get().isConnected())
		{
			reason = _("usb.not_connected");
		}
		else if (folderOf(static_cast<Destination>(m_destination.get())).empty())
		{
			reason = _("usb.no_folder");
		}
		show(m_upload, reason.empty() ? FilesSource::Control::Usable : FilesSource::Control::Disabled);
		m_uploadReason.set(reason);
	}

	void UsbSource::upload()
	{
		ZoneScoped;
		if (m_dialog.get() != "file" || m_upload.get() != static_cast<int32_t>(FilesSource::Control::Usable))
		{
			return;
		}
		const auto destination = static_cast<Destination>(m_destination.get());
		const std::string name(m_dialogTitle.get());
		const std::string target = folderOf(destination) + name;
		const uint64_t generation = m_generation;
		m_cancel = std::make_shared<std::atomic_bool>(false);
		setUploading(true, true); // nothing is sent while it is read
		m_uploadState.set(static_cast<int32_t>(UploadState::Reading));
		m_uploadText.set(_("usb.reading", name));
		m_dialog.set("upload");
		showAllowed();

		// A large file takes a while to read through for its size and CRC: not on the UI's thread
		std::weak_ptr<DataSource> self = weak_from_this();
		std::thread(
			[self, generation, path = m_path, target, cancel = m_cancel]
			{
				tracy::SetThreadName("Upload read");
				std::expected<Comm::FileContents, Comm::ReplyError> contents = Comm::FileContents::file(path, cancel);
				if (!contents)
				{
					if (auto source = self.lock())
					{
						static_cast<UsbSource&>(*source).uploadEnded(generation, contents.error().message);
					}
					return;
				}
				if (cancel->load())
				{
					// Cancelled as it was read through: nothing to send
					if (auto source = self.lock())
					{
						static_cast<UsbSource&>(*source).uploadEnded(generation,
																	 Comm::FileContents::cancelledError().message);
					}
					return;
				}
				if (auto source = self.lock())
				{
					static_cast<UsbSource&>(*source).uploadStarted(generation);
				}
				Comm::Connection::get().writeFile(target,
												  std::move(*contents),
												  [self, generation](std::expected<void, Comm::ReplyError> written)
												  {
													  if (auto source = self.lock())
													  {
														  static_cast<UsbSource&>(*source).uploadEnded(
															  generation, written ? "" : written.error().message);
													  }
												  });
			})
			.detach();
	}

	void UsbSource::uploadStarted(uint64_t generation)
	{
		UI_LOCK();
		if (generation != m_generation)
		{
			return;
		}
		if (m_cancel->load())
		{
			return; // still cancelling
		}
		setUploading(true, Comm::Connection::get().cancelsWrites());
		const std::string name(m_dialogTitle.get());
		m_uploadState.set(static_cast<int32_t>(UploadState::Sending));
		m_uploadText.set(_("usb.sending", name, destinationName(static_cast<Destination>(m_destination.get()))));
	}

	void UsbSource::uploadEnded(uint64_t generation, std::string error)
	{
		ZoneScoped;
		UI_LOCK();
		setUploading(false, false);
		showAllowed();
		if (generation != m_generation)
		{
			return; // the dialog has been opened for another file since
		}
		const std::string name(m_dialogTitle.get());
		if (!error.empty() && m_cancel->load())
		{
			LOG_INFO("Uploading {:s} was cancelled", m_path);
			m_uploadState.set(static_cast<int32_t>(UploadState::Cancelled));
			m_uploadText.set(_("usb.cancelled", name));
		}
		else if (error.empty())
		{
			LOG_INFO("Uploaded {:s}", m_path);
			m_uploadState.set(static_cast<int32_t>(UploadState::Done));
			m_uploadText.set(_("usb.done", name, destinationName(static_cast<Destination>(m_destination.get()))));
		}
		else
		{
			LOG_WARN("Couldn't upload {:s}: {:s}", m_path, error);
			m_uploadState.set(static_cast<int32_t>(UploadState::Failed));
			m_uploadText.set(_("usb.failed", name, error));
		}
	}

	void UsbSource::setUploading(bool uploading, bool cancellable)
	{
		m_uploading = uploading;
		m_cancellable.set(uploading && cancellable ? 1 : 0);
	}

	void UsbSource::cancelUpload()
	{
		ZoneScoped;
		if (m_cancellable.get() == 0)
		{
			return; // none going, already cancelled, or over USB or UART once it has started
		}
		LOG_INFO("Cancelling the upload of {:s}", m_path);
		m_cancel->store(true); // the file's contents share it, so stop being read
		m_cancellable.set(0);
		m_uploadState.set(static_cast<int32_t>(UploadState::Cancelling));
		m_uploadText.set(_("usb.cancelling", m_dialogTitle.get()));
	}

	void UsbSource::install()
	{
		ZoneScoped;
		if (m_dialog.get() != "file" || !m_updateInstallable)
		{
			return;
		}
		m_dialog.set("");
		m_installFailed.set(0);
		m_installText.set(_("usb.installing"));
		m_installing.set(1);
		std::weak_ptr<DataSource> self = weak_from_this();
		UpgradeHelper::installFromUsb(m_path,
									  [self](const std::string& why)
									  {
										  if (auto source = self.lock())
										  {
											  UI_LOCK();
											  auto& usb = static_cast<UsbSource&>(*source);
											  usb.m_installText.set(_("message.update_install_failed", why));
											  usb.m_installFailed.set(1);
										  }
									  });
	}

	void UsbSource::driveChanged()
	{
		ZoneScoped;
		update();
		// A file on a drive taken out can't be uploaded or installed; one being uploaded fails by itself
		const auto drives = USB::UsbMonitor::getInstance().getMountedDrives();
		const bool fileThere =
			std::ranges::any_of(drives, [this](const std::string& d) { return m_path.starts_with(d + "/"); });
		if (m_dialog.get() == "file" && !fileThere)
		{
			m_dialog.set("");
		}
	}
} // namespace UI::Xml
