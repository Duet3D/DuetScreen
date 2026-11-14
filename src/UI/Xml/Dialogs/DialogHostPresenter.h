/*
 * DialogHostPresenter.h
 *
 * The presenter of the dialog surface's chrome (assets/ui/components/dialogs/dialog_host.xml): what its header shows,
 * and its buttons. Each DialogHost makes its own and tells it what to show; without one (the UI API reference makes
 * it on its own) the buttons do nothing.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "helix_xml.h"

namespace UI::Xml
{
	class DialogHost;

	class DialogHostPresenter : public XmlPresenter
	{
	  public:
		/** A presenter for no surface: its buttons do nothing */
		DialogHostPresenter();

		/** Stop following the titles */
		~DialogHostPresenter() override;

		/** Declare the header's fields */
		void fields(FieldVisitor& f) override;

		/** Send the buttons to `host` */
		void attach(DialogHost* host) { m_host = host; }

		/**
		 * Show the header of a step: following its `title` and the title of the step it came from (`from`), either of
		 * which may be nullptr, with the back and close buttons if `canGoBack` and `canClose`
		 */
		void show(lv_subject_t* title, lv_subject_t* from, bool canGoBack, bool canClose);

	  private:
		/** Keeps a text subject a copy of another subject, as text */
		class Mirror
		{
		  public:
			/** Mirror into `copy` */
			explicit Mirror(StringSubject<128>& copy)
				: m_copy(copy)
			{
			}

			/** Stop following */
			~Mirror() { follow(nullptr); }

			Mirror(const Mirror&) = delete;
			Mirror& operator=(const Mirror&) = delete;

			/** Follow `source` (or nothing: the copy is then empty) */
			void follow(lv_subject_t* source);

		  private:
			/** The subject followed changed: copy it */
			static void changedCb(lv_observer_t* observer, lv_subject_t* subject);

			StringSubject<128>& m_copy;			// What the source is copied into
			lv_observer_t* m_observer = nullptr; // Watches the source, while there is one
		};

		DialogHost* m_host = nullptr; // Where the buttons go, or nullptr

		StringSubject<128> m_title;	  // `title`
		StringSubject<128> m_from;	  // `from`
		IntSubject m_canGoBack;		  // `can_go_back`
		IntSubject m_canClose;		  // `can_close`
		Mirror m_titleMirror{m_title}; // Keeps `title` the step's title
		Mirror m_fromMirror{m_from};   // Keeps `from` the previous step's title
	};
} // namespace UI::Xml
