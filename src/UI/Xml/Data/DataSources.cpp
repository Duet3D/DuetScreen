/*
 * DataSources.cpp
 */

#include "DataSources.h"
#include "AxesSource.h"
#include "BabystepSource.h"
#include "ConsoleSource.h"
#include "CustomiseSource.h"
#include "DataSource.h"
#include "Debug.h"
#include "ExtruderSource.h"
#include "FactorsSource.h"
#include "FilesSource.h"
#include "JobSource.h"
#include "JogSource.h"
#include "MachineSource.h"
#include "MoveSource.h"
#include "NavigationSource.h"
#include "ObjectModelSource.h"
#include "ResponseSource.h"
#include "SettingsSource.h"
#include "SpindleSource.h"
#include "SystemSource.h"
#include "TemperatureSource.h"
#include "ThemeSource.h"
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

		/** Publishes the fields it visits as global names, each after a source's prefix (`source.<name>.`) */
		class Publisher : public FieldVisitor
		{
		  public:
			/** Publish after `prefix` */
			explicit Publisher(std::string prefix)
				: m_prefix(std::move(prefix))
			{
			}

			/** Add the subject to the globals */
			void field(std::string_view name, Subject& subject, std::string_view) override
			{
				Globals::add(m_prefix + std::string(name), subject);
			}

			/** Add the list to the globals */
			void list(std::string_view name, ListBase& list, std::string_view) override
			{
				Globals::add(m_prefix + std::string(name), list);
			}

		  private:
			std::string m_prefix; // What each name is written after
		};

		/** Removes the fields it visits from the global names, each after a source's prefix */
		class Unpublisher : public FieldVisitor
		{
		  public:
			/** Remove names after `prefix` */
			explicit Unpublisher(std::string prefix)
				: m_prefix(std::move(prefix))
			{
			}

			/** Remove the subject from the globals */
			void field(std::string_view name, Subject&, std::string_view) override
			{
				Globals::remove(m_prefix + std::string(name));
			}

			/** Remove the list from the globals */
			void list(std::string_view name, ListBase&, std::string_view) override
			{
				Globals::remove(m_prefix + std::string(name));
			}

		  private:
			std::string m_prefix; // What each name is written after
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

		// The printer data available to XML
		s_sources = {
			std::make_shared<AxesSource>(),		   std::make_shared<BabystepSource>(),
			std::make_shared<ConsoleSource>(),	   std::make_shared<CustomiseSource>(),
			std::make_shared<ExtruderSource>(),	   std::make_shared<FactorsSource>(),
			std::make_shared<FilesSource>(),	   std::make_shared<ToolSource>(),
			std::make_shared<JobSource>(),		   std::make_shared<JogSource>(),
			std::make_shared<MachineSource>(),	   std::make_shared<MoveSource>(),
			std::make_shared<NavigationSource>(),  std::make_shared<ObjectModelSource>(),
			std::make_shared<ResponseSource>(),	   std::make_shared<SettingsSource>(),
			std::make_shared<SpindleSource>(),	   std::make_shared<SystemSource>(),
			std::make_shared<TemperatureSource>(), std::make_shared<ThemeSource>(),
		};

		for (auto& source : s_sources)
		{
			Publisher publisher(source->prefix());
			source->fields(publisher);
			source->activate();
			Model::get().bind(source);
			source->update();
		}
	}

	void stop()
	{
		ZoneScoped;
		for (auto& source : s_sources)
		{
			Model::get().unbind(source);
			source->deactivate();
			Unpublisher unpublisher(source->prefix());
			source->fields(unpublisher);
		}
		s_sources.clear();
	}

	const std::vector<std::shared_ptr<DataSource>>& all()
	{
		return s_sources;
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
