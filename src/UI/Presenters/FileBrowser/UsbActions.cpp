/*
 * UsbActions.cpp
 */

#include "UsbActions.h"
#include "Debug.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/UsbSource.h"
#include "UI/Xml/View/Actions.h"
#include <charconv>

namespace UI::UsbActions
{
	namespace
	{
		/** The USB file dialog's source; nullptr, with a warning, while the data sources aren't running */
		Xml::UsbSource* usb()
		{
			auto* source = Xml::DataSources::find<Xml::UsbSource>();
			if (source == nullptr)
			{
				LOG_WARN("The USB source isn't running");
			}
			return source;
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("usb.destination",
								"Upload the USB file dialog's file to the folder `arg`: 0 jobs, 1 macros, 2 system, "
								"3 firmware (source.usb.destination)",
								[](const Xml::ActionArgs& args)
								{
									int folder = -1;
									std::from_chars(args.arg.data(), args.arg.data() + args.arg.size(), folder);
									auto* source = usb();
									if (source == nullptr || folder < 0 || folder > 3)
									{
										return;
									}
									source->setDestination(static_cast<Xml::UsbSource::Destination>(folder));
								});
		Xml::Actions::addGlobal("usb.upload",
								"Upload the USB file dialog's file to the folder chosen; the dialog shows how it goes "
								"(source.usb.upload_state), and the upload carries on if it is closed",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = usb())
									{
										source->upload();
									}
								});
		Xml::Actions::addGlobal("usb.cancel_upload",
								"Cancel the upload from a USB drive that is going (source.usb.uploading): what was "
								"written of it is deleted",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = usb())
									{
										source->cancelUpload();
									}
								});
		Xml::Actions::addGlobal("usb.show_upload",
								"Show the last upload from a USB drive in the USB file dialog: how it is going (with "
								"Cancel upload, while it can be), or how it went",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = usb())
									{
										source->showUpload();
									}
								});
		Xml::Actions::addGlobal("usb.install",
								"Update the screen with the USB file dialog's file, an update archive found "
								"installable (source.usb.update); a dialog holds the screen until it restarts",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = usb())
									{
										source->install();
									}
								});
	}
} // namespace UI::UsbActions
