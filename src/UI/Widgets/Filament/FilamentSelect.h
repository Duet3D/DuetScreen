/*
 * FilamentSelect.h
 *
 *  Created on: 2025-07-15
 *      Author: Andy Everitt
 */

#pragma once

#include "FilamentSelectPresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/List/List.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Modal/Modal.h"
#include "UI/Core/View.h"

namespace UI
{
	class FilamentSelect : public View<FilamentSelectPresenter>
	{
	  public:
		FilamentSelect(const std::string& name, LvObj& parent, lv_obj_t* messageBoxParent = nullptr);

		void setToolCount(size_t count);
		void setToolData(size_t index, std::string_view toolName, std::string_view filamentName);
		void setFilamentOptions(const std::vector<std::string>& options);

		void showToolSelect(bool show);
		void showSelection(std::string_view toolName, std::string_view filamentName);
		void setSelectedFilament(std::string_view filamentName);

	  private:
		static void onFilamentOptionClicked(lv_event_t* e);

		void onShow() override;

		class ToolItem;

		LvLabel m_header;
		LvContainer m_cont;
		List<ToolItem> m_toolList;

		Modal<MessageBox> m_confirmation;
		List<Button> m_filamentOptions;
		Button m_unload;

		size_t m_selectedFilamentIndex;
	};
} // namespace UI
