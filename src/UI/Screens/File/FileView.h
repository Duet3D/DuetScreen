#pragma once

#include "FilePresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvImage.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/List/List.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Modal/Modal.h"
#include "UI/Core/View.h"
#include <memory>

namespace UI
{
	class FileView : public View<FilePresenter>
	{
	  public:
		class FileItem : public ListItem
		{
		  public:
			FileItem(const size_t index, LvObj& parent, FileView& view);
			void setFileLabel(const char* name);
			void setFileDate(const char* date);
			void setFileSize(const char* size);
			void setThumbnail(const char* thumbnail);
			void setType(const bool isFolder);

			const char* getLabel() const;
			const char* getDate() const;
			const char* getSize() const;

		  private:
			FileView& getList() const { return m_list; }

			static void onClick(lv_event_t* e);

			FileView& m_list;

			int32_t m_layoutColDsc[3];
			int32_t m_layoutRowDsc[4];

			LvLabel m_label{"label", getRoot()};
			LvLabel m_size{"size", getRoot()};
			LvLabel m_date{"date", getRoot()};
			LvImage m_thumbnail{"thumb", getRoot()};
			LvLabel m_type{"type", getRoot()};

			bool m_isFolder;
		};

		FileView(LvObj& parent, lv_obj_t* msgBoxParent = nullptr);

		const size_t getFileCount() const { return m_fileList.getItemCount(); }
		void setFileCount(const size_t count);
		std::shared_ptr<FileItem> getFileItem(size_t index) const;

		void setFolder(const std::string& path);
		bool cancelStartPrint();
		void confirmStartPrint(std::string_view filename,
							   std::string_view date,
							   std::string_view size,
							   std::string_view thumbnail);
		void confirmRunMacro(std::string_view filename);

		void showSort(FilePresenter::SortBy by, bool descending);

		void onItemClicked(size_t index, bool isFolder);

		bool back() override;

	  private:
		static void onRefreshClicked(lv_event_t* e);
		static void onSortClicked(lv_event_t* e);
		static void onBreadcrumbClicked(lv_event_t* e);

		virtual void onShow() override;
		virtual void onHide() override;

		LvContainer m_sideBar{"sidebar", getRoot()};
		List<FileItem> m_fileList{"list", getRoot()}; // manages header (breadcrumbs) + items container
		Button m_sortName{"sort_name", m_sideBar};
		Button m_sortDate{"sort_date", m_sideBar};
		Button m_sortSize{"sort_size", m_sideBar};
		LvContainer m_pad{"pad", m_sideBar};
		Button m_refresh{"refresh", m_sideBar};
		LvLabel m_footer{"footer", getRoot()};

		LvLabel m_breadcrumbPrefix{"breadcrumb_prefix", m_fileList.getHeader()};
		LvContainer m_breadcrumbCont{"breadcrumb_container", m_fileList.getHeader()};
		std::vector<std::string> m_breadcrumbPaths; // relative paths for each breadcrumb index
		std::vector<std::unique_ptr<Button>> m_breadcrumbButtons;
		std::vector<std::unique_ptr<LvLabel>> m_breadcrumbLabels;

		Modal<MessageBox> m_startPrint;
	};
} // namespace UI