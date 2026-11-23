/*
 * OmField.h
 *
 * The subject for one object model path read through one converter, e.g. `heat.heaters[1].current|int`. OmBindings
 * makes one the first time XML names it and keeps it, so every widget using that name shares it, and it stays valid
 * however the object model changes: while the field isn't there, it holds what the converter makes of null (empty
 * text, or 0).
 */

#pragma once

#include "Converters.h"
#include "ObjectModel/Tree/Path.h"
#include "UI/Xml/Binding/Subjects.h"
#include <memory>
#include <string>

namespace UI::Xml
{
	class OmField
	{
	  public:
		/** Longest text a text subject holds, terminator included; longer values are cut */
		static constexpr size_t TEXT_SIZE = 256;

		/**
		 * @param path The field
		 * @param converter What the subject holds
		 * @param argument The converter's argument
		 */
		OmField(OM::Path path, const Converter& converter, std::string argument);

		/** The subject widgets bind to */
		Subject& subject();

		/** The field */
		const OM::Path& path() const { return m_path; }

		/** Read the field from the object model `root` again (the model lock held) */
		void update(const OM::Node& root);

	  private:
		OM::Path m_path;										// The field
		const Converter& m_converter;							// What the subject holds
		std::string m_argument;									// The converter's argument
		std::unique_ptr<IntSubject> m_integer;					// The subject, for an integer converter
		std::unique_ptr<StringSubject<TEXT_SIZE>> m_text;		// The subject, for a text converter
	};
} // namespace UI::Xml
