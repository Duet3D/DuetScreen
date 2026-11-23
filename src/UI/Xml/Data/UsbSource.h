/*
 * UsbSource.h
 *
 * Publishes the USB drives plugged into the screen, and the shared dialog about a file on one, which the file browser
 * opens for a file tapped (assets/ui/components/modals/usb_dialog.xml, hosted by the home screen). The dialog offers to:
 *
 *   - upload the file to the printer, into the folder chosen (jobs, macros, system or firmware; the file's kind picks
 *     one at first), showing its progress (source.files.transfer_*) as it is sent;
 *   - update the screen with it, when it is an update archive (*.tar.gz) the screen's update service would install:
 *     it is checked as the dialog opens, and the dialog says why not if it can't be.
 *
 * Uploading carries on if the dialog is closed; one file at a time. It can be cancelled (cancelUpload()), from the
 * dialog or the file browser, while it is read or sent: what was written of it is deleted. Installing an update holds the screen in a dialog
 * of its own until the update service restarts it, or says why it couldn't.
 *
 * The global actions that answer the dialog are in Presenters/FileBrowser/UsbActions.h. The names it publishes are
 * declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "Comm/FileContents.h"
#include "DataSource.h"
#include "ObjectModel/Directories.h"
#include "ObjectModel/Files.h"
#include "UI/Xml/Binding/Subjects.h"
#include "utils/UpgradeHelper.h"
#include <cstdint>
#include <string>
#include <string_view>

namespace UI::Xml
{
	class UsbSource : public DataSource
	{
	  public:
		/** Where a file is uploaded to, as `destination` publishes it */
		enum class Destination : int32_t
		{
			Jobs = 0,	 // The jobs folder (gcodes)
			Macros = 1,	 // The macros folder
			System = 2,	 // The system folder (sys)
			Firmware = 3 // The firmware folder
		};

		/** How the file's upload is going, as `upload_state` publishes it */
		enum class UploadState : int32_t
		{
			Reading = 0, // Read through for its size and CRC, before it is sent
			Sending = 1, // Being sent (source.files.transfer_progress says how far)
			Done = 2,		// The printer has it
			Failed = 3,		// It couldn't be sent: `upload_text` says why
			Cancelling = 4, // Cancelled, and stopping (over USB and UART the printer is sent filler for the rest)
			Cancelled = 5	// Cancelled: none of it is left on the printer
		};

		/** Create the source. Nothing is published until DataSources::start(). */
		UsbSource();

		/** Declare `source.usb.*` */
		void fields(FieldVisitor& f) override;

		/** Count the drives again, and show what the dialog allows */
		void update() override;

		/** Show what the dialog allows: nothing is uploaded while the printer isn't connected */
		void clear() override;

		/** The folder `destination` is on the printer, as the printer last said, e.g. "0:/gcodes/"; empty if unknown */
		static std::string folderOf(Destination destination);

		/** The folder a file named `name` goes to at first: a macro (.g) to the macros, firmware (.bin, .uf2) to the
		 * firmware, settings and tables (.json, .csv) to the system folder, anything else to the jobs */
		static Destination destinationFor(std::string_view name);

		/** Ask what to do with the file `item`, on a USB drive */
		void ask(const OM::FileSystem::FileSystemItem& item);

		/** Upload the dialog's file to `destination` from now on */
		void setDestination(Destination destination);

		/** Upload the dialog's file to the folder chosen, showing its progress in the dialog */
		void upload();

		/** Cancel the upload going, if one is */
		void cancelUpload();

		/** Install the dialog's file, an update, holding the screen until it restarts */
		void install();

	  protected:
		/** Follow the drives coming and going, the printer's folders, and files being written */
		void onInit() override;

		/** Show what the dialog allows now the printer is connected */
		void onConnect() override { showAllowed(); }

		/** Show what the dialog allows now the printer isn't: the drives are the screen's, so stay */
		void onDisconnect() override { showAllowed(); }

	  private:
		/** A drive came or went: count them, and close the dialog if its file's drive went */
		void driveChanged();

		/** Show what the dialog allows: uploading needs the printer, its folder, room on the link, and no other
		 * upload; updating, an archive found installable */
		void showAllowed();

		/** The archive check of the dialog's file `generation` ended with `check` */
		void updateChecked(uint64_t generation, const UpgradeHelper::UpdateCheck& check);

		/** The upload of the dialog's file `generation` was read, and is being sent */
		void uploadStarted(uint64_t generation);

		/** The upload `generation` ended: the printer has it, or `error` says why not */
		void uploadEnded(uint64_t generation, std::string error);

		/** Note whether a file is being uploaded */
		void setUploading(bool uploading);

		std::string m_path;				  // The dialog's file, on its drive
		size_t m_size = 0;				  // Its size
		uint64_t m_generation = 0;		  // Counts the files the dialog has been about, so late answers are known
		bool m_updateInstallable = false; // The dialog's file is an update the screen can install
		bool m_uploading = false;		  // A file is being read or sent; only one at a time
		Comm::FileContents::CancelFlag m_cancel; // Cancels the upload going

		IntSubject m_drives;			   // `drives`
		StringSubject<16> m_dialog;		   // `dialog`: "file", "upload", or empty
		StringSubject<128> m_dialogTitle;  // `dialog_title`
		StringSubject<32> m_fileSize;	   // `file_size`
		StringSubject<32> m_fileDate;	   // `file_date`
		IntSubject m_destination;		   // `destination`: a Destination
		IntSubject m_upload;			   // `upload`: a FilesSource::Control
		StringSubject<128> m_uploadReason; // `upload_reason`
		IntSubject m_uploadState;		   // `upload_state`: an UploadState
		IntSubject m_uploadingSubject;	   // `uploading`
		StringSubject<256> m_uploadText;   // `upload_text`
		IntSubject m_update;			   // `update`: a FilesSource::Control
		StringSubject<384> m_updateText;   // `update_text`
		IntSubject m_installing;		   // `installing`
		StringSubject<256> m_installText;  // `install_text`
		IntSubject m_installFailed;		   // `install_failed`
	};
} // namespace UI::Xml
