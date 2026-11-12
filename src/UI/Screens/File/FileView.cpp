#include "FileView.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"
#include "lvgl/lvgl_private.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"

#define ENABLE_BREADCRUMB_SEPARATOR 1

namespace UI
{
	FileView::FileItem::FileItem(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
		, m_layoutColDsc{LV_GRID_FR(3), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST}
		, m_layoutRowDsc{LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST}
	{
		ZoneScoped;
		UI_LOCK();

		setWidth(LV_PCT(100));
		// setMaxWidth(400);

		setType(false);
		setDisplayMode(DisplayMode::List);

		m_label.setHeight(LV_SIZE_CONTENT);
		m_label.setWidth(LV_PCT(100));
		m_label.setLongMode(LV_LABEL_LONG_DOT);
		m_label.setStyleTextAlign(LV_TEXT_ALIGN_LEFT);

		m_thumbnail.setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);
		m_folderIcon.setIcon("folder.png");
		m_thumbnail.setHeight(72);
		m_thumbnail.setWidth(72);
		m_folderIcon.setSize(72, 72);

		addEventCallback(
			[this](lv_event_t*)
			{
				if (!m_fileView)
					return;
				m_fileView->onItemClicked(m_index, m_isFolder);
			},
			LV_EVENT_CLICKED);

		// Styles
		addStyle(Themes::getComponentStyles().file);
		addStyle(Themes::getComponentStyles().folder, LV_STATE_CHECKED);
		m_label.addStyle(Themes::getLvglStyles().text_emphasis, LV_STATE_CHECKED);
		m_date.addStyle(Themes::getLvglStyles().text_muted);
	}

	void FileView::FileItem::setDisplayMode(DisplayMode mode)
	{
		ZoneScoped;
		UI_LOCK();

		m_displayMode = mode;
		switch (mode)
		{
		case DisplayMode::Tile:
		{
			setLayoutStyle(LV_LAYOUT_FLEX);
			setFlexFlow(LV_FLEX_FLOW_COLUMN);
			setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
			setSize(150, 170);

			m_thumbnail.setWidth(LV_PCT(100));
			m_folderIcon.setWidth(LV_PCT(100));
			m_thumbnail.setFlexGrow(1);
			m_folderIcon.setFlexGrow(1);
			m_label.setLongMode(LV_LABEL_LONG_DOT);
			m_label.setFlexGrow(1);
			m_label.setMaxHeight(LV_SIZE_CONTENT);
			m_label.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
			m_date.setVisible(false);
			break;
		}
		case DisplayMode::List:
		{
			setLayoutStyle(LV_LAYOUT_GRID);
			setGridDsc(m_layoutColDsc, m_layoutRowDsc);
			setGridCell(m_label, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0, 1);
#if SHOW_FILE_ITEM_SIZE
			setGridCell(m_size, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_END, 2, 1);
#endif
			setGridCell(m_date, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_END, 3, 1);
			setGridCell(m_thumbnail, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_START, 0, 4);
			setGridCell(m_folderIcon, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_START, 0, 4);

			setWidth(LV_PCT(100));
			setHeight(90);
			m_thumbnail.setHeight(LV_PCT(100));
			m_folderIcon.setHeight(LV_PCT(100));
			m_thumbnail.setWidth(m_thumbnail.getHeight());
			m_folderIcon.setWidth(m_folderIcon.getHeight());
			m_label.setHeight(LV_SIZE_CONTENT);
			m_label.setStyleTextAlign(LV_TEXT_ALIGN_LEFT);
			m_date.setVisible(true);
			break;
		}
		}
	}

	void FileView::FileItem::setFileLabel(std::string_view name)
	{
		ZoneScoped;
		UI_LOCK();
		m_label.setText(name);
	}

	void FileView::FileItem::setFileDate(std::string_view date)
	{
		ZoneScoped;
		UI_LOCK();
		m_date.setText(date);
	}

#if SHOW_FILE_ITEM_SIZE
	void FileView::FileItem::setFileSize(std::string_view size)
	{
		ZoneScoped;
		UI_LOCK();
		m_size.setText(size);
	}
#endif

	void FileView::FileItem::setThumbnail(const char* thumbnail)
	{
		ZoneScoped;
		UI_LOCK();
		m_thumbnail.setSrc(thumbnail);
		switch (m_displayMode)
		{
		case DisplayMode::Tile:
			m_thumbnail.setWidth(LV_PCT(100));
			break;
		case DisplayMode::List:
			m_thumbnail.setWidth(thumbnail == nullptr ? 0 : m_thumbnail.getHeight());
			break;
		}
	}

	void FileView::FileItem::setType(const bool isFolder)
	{
		ZoneScoped;
		UI_LOCK();
		m_isFolder = isFolder;
#if SHOW_FILE_ITEM_SIZE
		m_size.setVisible(!isFolder);
#endif
		setState(LV_STATE_CHECKED, isFolder);
		m_label.setState(LV_STATE_CHECKED, isFolder);
		m_folderIcon.setVisible(isFolder);
		m_thumbnail.setVisible(!isFolder);
		switch (m_displayMode)
		{
		case DisplayMode::List:
			m_date.setVisible(true);
			break;
		case DisplayMode::Tile:
			m_date.setVisible(false);
			break;
		}
	}

	std::string_view FileView::FileItem::getLabel() const
	{
		ZoneScoped;
		UI_LOCK();
		return m_label.getText();
	}

	std::string_view FileView::FileItem::getDate() const
	{
		ZoneScoped;
		UI_LOCK();
		return m_date.getText();
	}

#if SHOW_FILE_ITEM_SIZE
	std::string_view FileView::FileItem::getSize() const
	{
		ZoneScoped;
		UI_LOCK();
		return m_size.getText();
	}
#endif

	FileView::LazyFileItem::LazyFileItem(FileView& fileView)
		: m_fileView(fileView)
	{
		ZoneScoped;
	}

	lv_coord_t FileView::LazyFileItem::getWidth() const
	{
		ZoneScoped;
		UI_LOCK();
		switch (m_fileView.getDisplayMode())
		{
		case DisplayMode::Tile:
			return 150;
		case DisplayMode::List:
			return 0;
		}

		std::unreachable();
	}

	lv_coord_t FileView::LazyFileItem::getHeight() const
	{
		ZoneScoped;
		UI_LOCK();
		switch (m_fileView.getDisplayMode())
		{
		case DisplayMode::Tile:
			return 170;
		case DisplayMode::List:
			return 90;
		}

		std::unreachable();
	}

	void FileView::LazyFileItem::update(size_t index, FileItem& obj)
	{
		ZoneScoped;
		UI_LOCK();

		obj.setFileView(&m_fileView);
		obj.setIndex(index);
		if (obj.getDisplayMode() != m_fileView.getDisplayMode())
		{
			obj.setDisplayMode(m_fileView.getDisplayMode());
		}
		if (m_filename != obj.getLabel())
		{
			obj.setFileLabel(m_filename);
		}
		std::string date(m_date);
		std::replace(date.begin(), date.end(), 'T', ' ');
		if (date != obj.getDate())
		{
			obj.setFileDate(date);
		}
#if SHOW_FILE_ITEM_SIZE
		obj.setFileSize(m_size);
#endif
		if (m_thumbnail != (obj.getThumbnail()))
		{
			obj.setThumbnail(m_thumbnail.empty() ? nullptr : m_thumbnail.c_str());
		}
		if (m_isFolder != obj.isFolder())
		{
			obj.setType(m_isFolder);
		}
	}

	FileView::StartPrintModal::StartPrintModal(const std::string& name, LvObj& parent)
		: Modal<MessageBox>(name, parent, layout_t(0, 0, 70, LV_SIZE_CONTENT))
	{
		ZoneScoped;
		m_fileInfoCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		static const int32_t colDsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		static const int32_t rowDsc[] = {LV_GRID_CONTENT,
										 LV_GRID_CONTENT,
										 LV_GRID_CONTENT,
										 LV_GRID_CONTENT,
										 LV_GRID_CONTENT,
										 LV_GRID_CONTENT,
										 LV_GRID_TEMPLATE_LAST};
		m_fileInfoCont.setGridDsc(colDsc, rowDsc);

		setGridCell(m_fileDateLabel, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
		setGridCell(m_fileDateValue, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 0, 1);

		setGridCell(m_fileSizeLabel, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 1, 1);
		setGridCell(m_fileSizeValue, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 1, 1);

		setGridCell(m_generatedByLabel, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);
		setGridCell(m_generatedByValue, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 2, 1);

		setGridCell(m_printTimeLabel, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 3, 1);
		setGridCell(m_printTimeValue, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 3, 1);

		setGridCell(m_heightLabel, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 4, 1);
		setGridCell(m_heightValue, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 4, 1);

		setGridCell(m_layerHeightLabel, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 5, 1);
		setGridCell(m_layerHeightValue, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 5, 1);

		m_fileDateLabel.setText(_("file.start_print.date"));
		m_fileSizeLabel.setText(_("file.start_print.size"));
		m_generatedByLabel.setText(_("file.start_print.generated_by"));
		m_printTimeLabel.setText(_("file.start_print.print_time"));
		m_heightLabel.setText(_("file.start_print.height"));
		m_layerHeightLabel.setText(_("file.start_print.layer_height"));

		m_fileDateLabel.addStyle(Themes::getLvglStyles().text_muted);
		m_fileSizeLabel.addStyle(Themes::getLvglStyles().text_muted);
		m_generatedByLabel.addStyle(Themes::getLvglStyles().text_muted);
		m_printTimeLabel.addStyle(Themes::getLvglStyles().text_muted);
		m_heightLabel.addStyle(Themes::getLvglStyles().text_muted);
		m_layerHeightLabel.addStyle(Themes::getLvglStyles().text_muted);

		m_okBtn.addStyle(Themes::getLvglStyles().actionBtn);

		m_deleteBtn.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_deleteBtn.addClickedCallback(
			[this](lv_event_t*)
			{
				if (m_deleteCb)
				{
					m_deleteCb();
				}
			});
		m_deleteBtn.setVisible(false);
		m_deleteBtn.moveToIndex(0); // position on the left
	}

	void FileView::StartPrintModal::setFile(std::string_view value)
	{
		ZoneScoped;
		setText(value);
	}

	void FileView::StartPrintModal::setFileDate(std::string_view value)
	{
		ZoneScoped;
		m_fileDateValue.setText(value);
	}

	void FileView::StartPrintModal::setFileSize(std::string_view value)
	{
		ZoneScoped;
		m_fileSizeValue.setText(value);
	}

	void FileView::StartPrintModal::setGeneratedBy(std::string_view value)
	{
		ZoneScoped;
		m_generatedByValue.setText(value);
	}

	void FileView::StartPrintModal::setPrintTime(std::string_view value)
	{
		ZoneScoped;
		m_printTimeValue.setText(value);
	}

	void FileView::StartPrintModal::setHeight(float value)
	{
		ZoneScoped;
		m_heightValue.setText(fmt::format("{:g} {:s}", value, Units::getDisplayedDistanceUnit()));
	}

	void FileView::StartPrintModal::setLayerHeight(float value)
	{
		ZoneScoped;
		m_layerHeightValue.setText(fmt::format("{:g} {:s}", value, Units::getDisplayedDistanceUnit()));
	}

	FileView::FileView(const std::string& name, LvObj& parent, StorageKeys storageKeys)
		: View(name, parent, layout_t(0, 0, 100, 100))
		, m_storageKeys(std::move(storageKeys))
	{
		ZoneScoped;
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);

		// Layout
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		// Sidebar
		m_sortDate.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_sortName.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_sortSize.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_pad.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_pad.setFlexGrow(1);
		m_displayModeBtn.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_refresh.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		m_sortDate.setText(_("file.sort_by_name"));
		m_sortName.setText(_("file.sort_by_date"));
		m_sortSize.setText(_("file.sort_by_size"));
		m_displayModeBtn.setText(LV_SYMBOL_IMAGE);
		m_refresh.setText(_("file.refresh"));

		// Header (from List) becomes breadcrumb container
		m_fileList.getHeader().setFlexFlow(LV_FLEX_FLOW_ROW);
		m_fileList.getHeader().setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_fileList.showHeader(true);
		m_breadcrumbPrefix.setText(_("file.header_prefix"));
		m_breadcrumbPrefix.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_breadcrumbCont.setHeight(LV_SIZE_CONTENT);
		m_breadcrumbCont.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);
		m_breadcrumbCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
		m_breadcrumbCont.setFlexGrow(1);
		m_breadcrumbCont.addStyle(Themes::getLvglStyles().pad_tiny);
		setFolder("");

		// List container styling
		m_fileList.setWidth(LV_PCT(100));
		m_fileList.setFlexGrow(1);
		// m_fileList.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
		// m_fileList.getListContainer().setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

		// Sidebar
		m_sideBar.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_sideBar.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_sideBar.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_sortName.setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(FilePresenter::SortBy::NAME)));
		m_sortDate.setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(FilePresenter::SortBy::DATE)));
		m_sortSize.setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(FilePresenter::SortBy::SIZE)));
		m_sortName.setCheckable(true);
		m_sortDate.setCheckable(true);
		m_sortSize.setCheckable(true);
		m_displayModeBtn.setCheckable(true);

		// Footer
		m_footer.hide();

		// Start Print
		m_startPrint.setFlag(LV_OBJ_FLAG_FLOATING, true);
		m_startPrint.setAlign(LV_ALIGN_CENTER, 0, 0);
		m_startPrint.setMaxHeight(LV_PCT(70));
		m_startPrint.okVisible(true);
		m_startPrint.cancelVisible(true);
		m_startPrint.setImageSize(LV_PCT(50), 200);
		m_startPrint.setCloseCallback([this]() { closeModal(&m_startPrint); });

		// Callbacks
		m_refresh.addClickedCallback(onRefreshClicked, this);
		m_sortName.addClickedCallback(onSortClicked, this);
		m_sortDate.addClickedCallback(onSortClicked, this);
		m_sortSize.addClickedCallback(onSortClicked, this);
		m_displayModeBtn.addClickedCallback(onDisplayModeClicked, this);

		setDisplayMode(StorageHelper::getData(m_storageKeys.displayMode));
	}

	void FileView::setFileCount(const size_t count)
	{
		ZoneScoped;
		UI_LOCK();
		m_fileList.setItemCount(count,
								[this](size_t)
								{
									auto item = std::make_unique<LazyFileItem>(*this);
									return item;
								});
		// m_fileList.setItemCount(count, *this);
	}

	void FileView::setFolder(const std::string& folder)
	{
		ZoneScoped;
		UI_LOCK();
		// Clear previous breadcrumb elements
		m_breadcrumbButtons.clear();
		m_breadcrumbLabels.clear();
		m_fileList.showHeader(true); // ensure visible

		m_breadcrumbPaths.clear();
		std::string_view basePath = m_presenter->getBaseFolderPath();

		// Remove base path prefix from full path to get relative path
		std::string_view relPath = folder;
		std::vector<std::string_view> segments;
		if (!basePath.empty() && relPath.starts_with(basePath))
		{
			segments.emplace_back(basePath);
			relPath = relPath.substr(basePath.size());
			if (!relPath.empty() && relPath.front() == '/')
			{
				relPath.remove_prefix(1);
			}
		}

		if (!relPath.empty())
		{
			size_t start = 0;
			while (start < relPath.size())
			{
				size_t end = relPath.find('/', start);
				if (end == std::string_view::npos)
					end = relPath.size();
				if (end > start)
				{
#if ENABLE_BREADCRUMB_SEPARATOR
					segments.emplace_back(relPath.substr(start, end - start));
#else
					segments.emplace_back(relPath.substr(start, end - start + 1));
#endif
				}
				start = end + 1;
			}
		}

		std::string accum;
		m_breadcrumbButtons.reserve(segments.size());
		for (size_t i = 0; i < segments.size(); ++i)
		{
			if (!accum.empty())
			{
				accum += '/';
			}
			accum += segments[i];
			m_breadcrumbPaths.push_back(accum); // relative path accumulation
		}

		m_breadcrumbLabels.reserve(segments.size());
		m_breadcrumbButtons.reserve(segments.size());
		for (size_t i = 0; i < segments.size(); ++i)
		{
#if ENABLE_BREADCRUMB_SEPARATOR
			if (i > 0)
			{
				auto sep = std::make_unique<LvLabel>(fmt::format("crumb_sep_{}", i), m_breadcrumbCont);
				sep->setText(_("file.crumb_separator"));
				m_breadcrumbLabels.push_back(std::move(sep));
			}
#endif // ENABLE_BREADCRUMB_SEPARATOR
			if (i == segments.size() - 1)
			{
				// Last segment is not clickable
				auto label = std::make_unique<LvLabel>(fmt::format("crumb_label_{}", i), m_breadcrumbCont);
				label->setText(segments[i]);
				m_breadcrumbLabels.push_back(std::move(label));
				continue;
			}

			auto btn = std::make_unique<Button>(fmt::format("crumb_btn_{}", i), m_breadcrumbCont, segments[i]);
			btn->setUserData(reinterpret_cast<void*>(i));
			btn->addClickedCallback(onBreadcrumbClicked, this);
			m_breadcrumbButtons.push_back(std::move(btn));
		}
	}

	void FileView::onBreadcrumbClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		FileView* view = static_cast<FileView*>(lv_event_get_user_data(e));
		LvObj* btn = LvObj::fromPtr(lv_event_get_target_obj(e));
		size_t index = reinterpret_cast<size_t>(btn->getUserData());
		if (index >= view->m_breadcrumbPaths.size())
		{
			return;
		}
		// Set folder relative path using presenter->setFolder
		view->m_presenter->setFolder(view->m_breadcrumbPaths[index]);
	}

	void FileView::onItemClicked(size_t index, bool /* isFolder */)
	{
		ZoneScoped;
		UI_LOCK();
		if (index >= m_fileList.getLazyItemCount())
		{
			return;
		}

		m_presenter->itemClicked(index);
	}

	void FileView::confirmDelete(std::string_view filename)
	{
		ZoneScoped;
		UI_LOCK();
		m_startPrint.setTitle(_("file.delete_title"));
		m_startPrint.setText(_("file.delete_message", filename));
		m_startPrint.setOkCallback([this]() { m_presenter->deleteSelectedItem(); });
		m_startPrint.deleteVisible(false);
		m_startPrint.setDeleteCallback(nullptr);
		m_startPrint.setImage(nullptr);
		m_startPrint.getFileInfo().hide();
		openModal(&m_startPrint);
	}

	bool FileView::cancelStartPrint()
	{
		ZoneScoped;
		UI_LOCK();
		if (m_startPrint.isVisible())
		{
			m_startPrint.cancel();
			return true;
		}
		return false;
	}

	void FileView::confirmStartPrint(std::string_view filename, const std::filesystem::path& thumbnail)
	{
		ZoneScoped;
		UI_LOCK();
		m_startPrint.setTitle(_("file.start_print_title"));
		m_startPrint.setText(_("file.start_print.file", filename));
		m_startPrint.setOkCallback([this]() { m_presenter->startPrint(); });
		m_startPrint.setDeleteCallback([this, capturedFilename = std::string(filename)]()
									   { confirmDelete(capturedFilename); });
		m_startPrint.deleteVisible(true);
		m_startPrint.setImage(IsThumbnailCached(thumbnail) ? thumbnail.c_str() : nullptr);
		openModal(&m_startPrint);
	}

	void FileView::confirmRunMacro(std::string_view filename)
	{
		ZoneScoped;
		UI_LOCK();
		m_startPrint.deleteVisible(false);
		m_startPrint.setDeleteCallback(nullptr);
		m_startPrint.setTitle(_("file.run_macro_title"));
		m_startPrint.setText(_("file.run_macro_message", filename));
		m_startPrint.setOkCallback([this]() { m_presenter->runMacro(); });
		m_startPrint.setImage(nullptr);
		m_startPrint.getFileInfo().hide();
		openModal(&m_startPrint);
	}

	void FileView::showSort(FilePresenter::SortBy by, bool descending)
	{
		ZoneScoped;
		m_sortName.setChecked(false);
		m_sortDate.setChecked(false);
		m_sortSize.setChecked(false);

		m_sortName.setText(_("file.sort_by_name"));
		m_sortDate.setText(_("file.sort_by_date"));
		m_sortSize.setText(_("file.sort_by_size"));

		switch (by)
		{
		case FilePresenter::SortBy::NAME:
			m_sortName.setChecked(true);
			m_sortName.setText(descending ? fmt::format(LV_SYMBOL_DOWN " {:s}", _("file.sort_by_name"))
										  : fmt::format(LV_SYMBOL_UP " {:s}", _("file.sort_by_name")));
			break;
		case FilePresenter::SortBy::DATE:
			m_sortDate.setChecked(true);
			m_sortDate.setText(descending ? fmt::format(LV_SYMBOL_DOWN " {:s}", _("file.sort_by_date"))
										  : fmt::format(LV_SYMBOL_UP " {:s}", _("file.sort_by_date")));
			break;
		case FilePresenter::SortBy::SIZE:
			m_sortSize.setChecked(true);
			m_sortSize.setText(descending ? fmt::format(LV_SYMBOL_DOWN " {:s}", _("file.sort_by_size"))
										  : fmt::format(LV_SYMBOL_UP " {:s}", _("file.sort_by_size")));
			break;
		}
	}

	void FileView::onRefreshClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		FileView* view = static_cast<FileView*>(lv_event_get_user_data(e));
		view->cancelStartPrint();
		view->m_presenter->refreshFiles();
	}

	void FileView::onSortClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		FileView* view = static_cast<FileView*>(lv_event_get_user_data(e));
		LvObj* btn = LvObj::fromPtr(lv_event_get_target_obj(e));
		FilePresenter::SortBy sort =
			static_cast<FilePresenter::SortBy>(reinterpret_cast<uintptr_t>(btn->getUserData()));

		bool forward = view->m_presenter->getSortOrder();
		if (view->m_presenter->getSortBy() == sort)
		{
			forward = !forward;
		}
		view->m_presenter->setSort(sort, forward);
	}

	void FileView::onDisplayModeClicked(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		FileView* view = static_cast<FileView*>(lv_event_get_user_data(e));
		switch (view->m_displayMode)
		{
		case DisplayMode::List:
			view->setDisplayMode(DisplayMode::Tile);
			break;
		case DisplayMode::Tile:
			view->setDisplayMode(DisplayMode::List);
			break;
		}
	}

	void FileView::setDisplayMode(DisplayMode mode)
	{
		ZoneScoped;
		UI_LOCK();

		m_displayMode = mode;
		switch (mode)
		{
		case DisplayMode::Tile:
			m_fileList.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
			StorageHelper::setData(m_storageKeys.displayMode, mode);
			break;
		case DisplayMode::List:
			m_fileList.setListFlow(LV_FLEX_FLOW_COLUMN);
			StorageHelper::setData(m_storageKeys.displayMode, mode);
			break;
		}

		updateDisplayModeButton();
		m_fileList.refresh();
	}

	void FileView::updateDisplayModeButton()
	{
		ZoneScoped;
		UI_LOCK();
		switch (m_displayMode)
		{
		case DisplayMode::Tile:
			m_displayModeBtn.setChecked(true);
			m_displayModeBtn.setText(LV_SYMBOL_LIST);
			break;
		case DisplayMode::List:
			m_displayModeBtn.setChecked(false);
			m_displayModeBtn.setText(LV_SYMBOL_IMAGE);
			break;
		}
	}

	bool FileView::back()
	{
		ZoneScoped;
		UI_LOCK();
		if (cancelStartPrint())
		{
			return true;
		}
		return m_presenter->back();
	}

	void FileView::onInit()
	{
		ZoneScoped;
		if (LvObj* host = getModalHost())
		{
			m_startPrint.setParent(*host);
		}
	}

	void FileView::onShow()
	{
		ZoneScoped;
		UI_LOCK();
		cancelStartPrint();
	}

	void FileView::onHide()
	{
		ZoneScoped;
	}
} // namespace UI
