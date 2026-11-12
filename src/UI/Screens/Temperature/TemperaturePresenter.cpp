#include "TemperaturePresenter.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Tool.h"
#include "TemperatureView.h"
#include "UI/Core/Navigation.h"
#include "UI/Widgets/Control/ExtrusionFactor/ExtrusionFactor.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"

namespace UI
{
	void TemperaturePresenter::newToolData()
	{
		ZoneScoped;
		MODEL_LOCK();
		auto& tools = m_view->getTools();
		std::vector<size_t> toolIndices;
		toolIndices.reserve(OM::GetToolCount());
		OM::IterateToolsWhile(
			[&toolIndices](OM::ToolPtr tool, size_t index)
			{
				if (tool->GetHeaterCount() > 0)
				{
					toolIndices.push_back(index);
				}
				return true;
			});

		tools.setItemCount(toolIndices.size(),
						   [this, &toolIndices](size_t index, LvObj& parent)
						   {
							   auto control = std::make_unique<ToolControl>(fmt::format("{}", index), parent);
							   control->getPresenter()->setToolIndex(toolIndices[index]);
							   control->setNumberPad(&m_view->getNumberPad());
							   control->setExtrusionModal(findView<ModalExtrusionFactor>("extrusion_factor"));
							   control->addStyle(Themes::getLvglStyles().card);
							   control->show(); // show recursively activates presenters
							   return control;
						   });

		if (auto current_tool = OM::GetCurrentTool())
		{
			getView()->setSelectedToolName(current_tool->GetName());
		}
		else
		{
			getView()->setSelectedToolName("");
		}
	}

	void TemperaturePresenter::newBedHeaterData()
	{
		ZoneScoped;
		auto& beds = m_view->getBeds();
		beds.setItemCount(OM::GetBedCount(),
						  [this](size_t index, LvObj& parent)
						  {
							  auto control = std::make_unique<HeaterSlider>(fmt::format("{}", index), parent);
							  control->getPresenter()->setBedIndex(index);
							  control->setNumberPad(&m_view->getNumberPad());
							  control->addStyle(Themes::getLvglStyles().card);
							  control->show(); // show recursively activates presenters
							  return control;
						  });
	}

	void TemperaturePresenter::newChamberHeaterData()
	{
		ZoneScoped;
		auto& chambers = m_view->getChambers();
		chambers.setItemCount(OM::GetChamberCount(),
							  [this](size_t index, LvObj& parent)
							  {
								  auto control = std::make_unique<HeaterSlider>(fmt::format("{}", index), parent);
								  control->getPresenter()->setChamberIndex(index);
								  control->setNumberPad(&m_view->getNumberPad());
								  control->addStyle(Themes::getLvglStyles().card);
								  control->show(); // show recursively activates presenters
								  return control;
							  });
	}

	void TemperaturePresenter::onDisconnect()
	{
		ZoneScoped;
		m_view->getTools().clear();
		m_view->getBeds().clear();
		m_view->getChambers().clear();
	}

	void TemperaturePresenter::onActivate()
	{
		ZoneScoped;
		MODEL_LOCK();

		newToolData();
		newBedHeaterData();
		newChamberHeaterData();

		OM::FileSystem::RequestFilaments();
	}

	void TemperaturePresenter::onDeactivate() {}
} // namespace UI
