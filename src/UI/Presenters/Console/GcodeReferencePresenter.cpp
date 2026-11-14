/*
 * GcodeReferencePresenter.cpp
 */

#include "GcodeReferencePresenter.h"
#include "Debug.h"
#include "Gcodes.h"
#include "UI/Xml/Data/ConsoleSource.h"
#include "UI/Xml/Data/DataSources.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace UI
{
	namespace
	{
		/** `text` in upper case */
		std::string upper(std::string_view text)
		{
			std::string result(text);
			std::transform(
				result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
			return result;
		}

		/** The console's source, which holds what is typed; nullptr while the data sources aren't running */
		Xml::ConsoleSource* console()
		{
			return Xml::DataSources::find<Xml::ConsoleSource>();
		}
	} // namespace

	void GcodeRow::fields(Xml::FieldVisitor& f)
	{
		f.field("gcode", gcode, "The G-code, e.g. \"G28\"");
		f.field("description", description, "What it does, e.g. \"Home\"");
	}

	GcodeReferencePresenter::GcodeReferencePresenter()
		: XmlPresenter("gcode_reference")
	{
		action("reference.open",
			   "Open the G-code reference dialog, searched for the first word typed in the console",
			   [this](const Xml::ActionArgs&)
			   {
				   const std::string typed(console() != nullptr ? console()->input() : "");
				   const std::string word = typed.substr(0, typed.find(' '));
				   m_search.set(word);
				   filter(word);
				   m_open.set(1);
			   });
		action("reference.search",
			   "Search the reference for the search box's text (`value_changed` on it)",
			   [this](const Xml::ActionArgs& args)
			   {
				   m_search.set(args.text());
				   filter(args.text());
			   });
		action("reference.pick",
			   "Type the G-code in the console's input, and close the reference dialog. From a row of `gcodes`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<GcodeRow>(); row != nullptr && console() != nullptr)
				   {
					   console()->setInput(row->gcode.get());
				   }
				   m_open.set(0);
			   });
		filter("");
	}

	void GcodeReferencePresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("search", m_search, "What the reference is searched for");
		f.field("open", m_open, "1 while the reference dialog is open");
		f.list("gcodes",
			   m_gcodes,
			   "The G-codes the search matches: their code starts with it, or their description contains it, ignoring "
			   "case; every G-code while the search is empty");
	}

	void GcodeReferencePresenter::filter(std::string_view search)
	{
		ZoneScoped;
		const std::string wanted = upper(search);

		struct Match
		{
			const gcode* code;		 // The G-code
			std::string description; // What it does, translated
		};
		std::vector<Match> matches;
		for (size_t i = 0; i < Gcodes::getGcodeCount(); i++)
		{
			const gcode* g = Gcodes::getGcode(i);
			std::string description(_(g->helpKey));
			if (g->gcode.starts_with(wanted) || upper(description).find(wanted) != std::string::npos)
			{
				matches.push_back({g, std::move(description)});
			}
		}
		m_gcodes.sync(matches.size(),
					  [&](GcodeRow& row, size_t i)
					  {
						  row.gcode.set(matches[i].code->gcode);
						  row.description.set(matches[i].description);
					  });
	}
} // namespace UI
