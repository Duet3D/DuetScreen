/*
 * DataSources.cpp
 */

#include "DataSources.h"
#include "DataSource.h"
#include "Debug.h"
#include "FanSource.h"
#include "HeatSource.h"
#include "MachineSource.h"
#include "NavigationSource.h"
#include "ResponseSource.h"
#include "ToolSource.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Binding/Globals.h"
#include <memory>
#include <vector>

namespace UI::Xml::DataSources
{
	namespace
	{
		std::vector<std::shared_ptr<DataSource>> s_sources; // Live sources, between start() and stop()

		/** Publishes the fields it visits as global names */
		class Publisher : public FieldVisitor
		{
		  public:
			/** Add the subject to the globals */
			void field(std::string_view name, Subject& subject, std::string_view) override
			{
				Globals::add(std::string(name), subject);
			}

			/** Add the list to the globals */
			void list(std::string_view name, ListBase& list, std::string_view) override
			{
				Globals::add(std::string(name), list);
			}
		};

		/** Removes the fields it visits from the global names */
		class Unpublisher : public FieldVisitor
		{
		  public:
			/** Remove the subject from the globals */
			void field(std::string_view name, Subject&, std::string_view) override
			{
				Globals::remove(std::string(name));
			}

			/** Remove the list from the globals */
			void list(std::string_view name, ListBase&, std::string_view) override
			{
				Globals::remove(std::string(name));
			}
		};
	} // namespace

	void start()
	{
		ZoneScoped;
		if (!s_sources.empty())
		{
			LOG_WARN("Data sources already started");
			return;
		}

		// The printer data available to XML. Keep this list in step with docs/XML_UI.md.
		s_sources = {
			std::make_shared<FanSource>(),
			std::make_shared<ToolSource>(),
			std::make_shared<HeatSource>(),
			std::make_shared<MachineSource>(),
			std::make_shared<NavigationSource>(),
			std::make_shared<ResponseSource>(),
		};

		Publisher publisher;
		for (auto& source : s_sources)
		{
			source->fields(publisher);
			source->activate();
			Model::get().bind(source);
			source->update();
		}
	}

	void stop()
	{
		ZoneScoped;
		Unpublisher unpublisher;
		for (auto& source : s_sources)
		{
			Model::get().unbind(source);
			source->deactivate();
			source->fields(unpublisher);
		}
		s_sources.clear();
	}

	void updateAll()
	{
		ZoneScoped;
		for (auto& source : s_sources)
		{
			source->update();
		}
	}
} // namespace UI::Xml::DataSources
