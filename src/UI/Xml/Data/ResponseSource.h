/*
 * ResponseSource.h
 *
 * Publishes that the printer replied, so a control can draw attention to the console, e.g.
 * `<ds_button flash_on="responses.count"/>`
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"
#include <string>

enum class ResponseType;

namespace UI::Xml
{
	class ResponseSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		ResponseSource();

		/** Declare `responses.count` */
		void fields(FieldVisitor& f) override;

		/** Nothing to re-read: responses are events */
		void update() override {}

		/** Nothing to forget: the count only ever goes up */
		void clear() override {}

	  protected:
		/** Listen for responses */
		void onInit() override;

	  private:
		/** Count a response */
		void received(const ResponseType type, const std::string& response);

		IntSubject m_count; // Responses received
	};
} // namespace UI::Xml
