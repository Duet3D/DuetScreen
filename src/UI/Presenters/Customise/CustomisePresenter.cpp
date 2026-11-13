/*
 * CustomisePresenter.cpp
 */

#include "CustomisePresenter.h"
#include "Debug.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Layout/WidgetCatalogue.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace UI
{
	namespace
	{
		/** The editor */
		Xml::LayoutEditor& editor()
		{
			return Xml::LayoutEditor::get();
		}

		/** `tag` translated, or `fallback` if there is no tag */
		std::string shown(const std::string& tag, const std::string& fallback)
		{
			return tag.empty() ? fallback : std::string(_(tag.c_str()));
		}

		/** The shared customise presenter: the view called `customise`'s; nullptr if there is none */
		CustomisePresenter* sharedCustomise()
		{
			auto* view = findView<Xml::XmlView>("customise");
			auto* presenter = view != nullptr ? dynamic_cast<CustomisePresenter*>(view->getPresenter()) : nullptr;
			if (presenter == nullptr)
			{
				LOG_WARN("There is no customise toolbar (a view called 'customise' showing customise.xml)");
			}
			return presenter;
		}

		/** The selected widget's definition, or nullptr */
		const Xml::WidgetDef* selectedWidget()
		{
			const Xml::LayoutItem* item = editor().selectedItem();
			return item ? Xml::WidgetCatalogue::find(item->widget) : nullptr;
		}

		/** The current value of `setting` of the selected item: its own, else the default */
		nlohmann::json currentValue(const Xml::SettingDef& setting)
		{
			const Xml::LayoutItem* item = editor().selectedItem();
			if (item != nullptr && item->settings.contains(setting.name))
			{
				if (auto value = setting.accept(item->settings[setting.name]))
				{
					return *value;
				}
			}
			return setting.fallback;
		}

		/** `value` of `setting` as the user sees it, e.g. "60 s", "Fan 1" */
		std::string shownValue(const Xml::SettingDef& setting, const nlohmann::json& value)
		{
			const std::string text = Xml::SettingDef::toProp(value);
			switch (setting.kind)
			{
			case Xml::SettingKind::Int:
				return setting.unit.empty() ? text : text + " " + setting.unit;
			case Xml::SettingKind::Choice:
			case Xml::SettingKind::Row:
				for (const auto& option : setting.choices())
				{
					if (option.value == text)
					{
						return shown(option.titleTag, option.title);
					}
				}
				return text;
			default:
				return text;
			}
		}
	} // namespace

	void CatalogueRow::fields(Xml::FieldVisitor& f)
	{
		f.field("id", id, "The widget's id");
		f.field("title", title, "Its name, translated");
		f.field("description", description, "What it shows, translated");
		f.field("icon", icon, "Its icon, e.g. \"fan.png\"; empty for none");
	}

	void CategoryRow::fields(Xml::FieldVisitor& f)
	{
		f.field("title", title, "The category's name, translated");
		f.list("widgets", widgets, "Its widgets that fit where one is being added, by name");
	}

	void SettingRow::fields(Xml::FieldVisitor& f)
	{
		f.field("name", name, "The setting's name");
		f.field("title", title, "Its name as shown, translated");
		f.field("kind", kind, "How it is chosen: int, bool, text, choice or row");
		f.field("control",
				control,
				"What changes it: `button` (showing the value; it opens the number pad or the values), `switch` or "
				"`text` (typed in place)");
		f.field("value", value, "Its value as shown, e.g. \"Fan 1\"");
		f.field("on", on, "For a bool, 1 when on, else 0");
	}

	void OptionRow::fields(Xml::FieldVisitor& f)
	{
		f.field("value", value, "The value");
		f.field("title", title, "The value as shown, translated");
		f.field("chosen", chosen, "1 for the setting's current value, else 0");
	}

	CustomisePresenter::CustomisePresenter()
		: XmlPresenter("customise")
	{
		editor().addListener(this);

		action("customise.add",
			   "Add the widget of the widget pool's row where it was asked for, and close the pool",
			   [this](const Xml::ActionArgs& args)
			   {
				   setOpen(m_catalogueOpen, 0);
				   if (const auto* row = args.row<CatalogueRow>())
				   {
					   editor().add(std::string(row->id.get()));
				   }
			   });
		action("customise.close_catalogue",
			   "Close the widget pool",
			   [this](const Xml::ActionArgs&) { setOpen(m_catalogueOpen, 0); });
		action("customise.configure",
			   "Show the selected widget's settings",
			   [this](const Xml::ActionArgs&)
			   {
				   showSettings();
				   setOpen(m_settingsOpen, editor().canConfigure() ? 1 : 0);
			   });
		action("customise.close_settings",
			   "Close the settings",
			   [this](const Xml::ActionArgs&)
			   {
				   m_keyboard.set(0);
				   setOpen(m_settingsOpen, 0);
			   });
		action("customise.edit_setting",
			   "Change the setting of the row: on the number pad, a choice of its values, or typed",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<SettingRow>())
				   {
					   editSetting(*row);
				   }
			   });
		action("customise.toggle_setting",
			   "Turn the on or off setting of the row the other way",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<SettingRow>())
				   {
					   setSetting(std::string(row->name.get()), row->on.get() == 0);
				   }
			   });
		action("customise.choose",
			   "Set the setting being changed to the value of the row, and close the values",
			   [this](const Xml::ActionArgs& args)
			   {
				   setOpen(m_optionsOpen, 0);
				   if (const auto* row = args.row<OptionRow>())
				   {
					   setSetting(m_choosing, std::string(row->value.get()));
				   }
			   });
		action("customise.close_options",
			   "Close the values without changing the setting",
			   [this](const Xml::ActionArgs&) { setOpen(m_optionsOpen, 0); });
		action("customise.set_text",
			   "Set the text setting of the row to the text typed (the text area's), e.g. when the keyboard's OK is "
			   "pressed",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<SettingRow>())
				   {
					   setSetting(std::string(row->name.get()), args.text());
				   }
			   });
		action("customise.keyboard",
			   "Set `keyboard` to `arg`: 1 when the keyboard comes up for a text setting, 0 when it goes away",
			   [this](const Xml::ActionArgs& args) { m_keyboard.set(args.argInt().value_or(0) != 0 ? 1 : 0); });
		action("customise.remove",
			   "Remove the selected widget",
			   [](const Xml::ActionArgs&) { editor().removeSelected(); });
		action("customise.undo", "Undo the last change", [](const Xml::ActionArgs&) { editor().undo(); });
		action("customise.reset",
			   "Put the selected widget's slot (else the dashboard) back as it was before any customising",
			   [](const Xml::ActionArgs&) { editor().reset(editor().targetSlot()); });
		action("customise.grid",
			   "Show the grid's size",
			   [this](const Xml::ActionArgs&)
			   {
				   showGrid();
				   setOpen(m_gridOpen, 1);
			   });
		action("customise.close_grid",
			   "Close the grid's size",
			   [this](const Xml::ActionArgs&) { setOpen(m_gridOpen, 0); });
		action("customise.columns",
			   "Ask for the grid's number of columns on the number pad",
			   [this](const Xml::ActionArgs&)
			   {
				   const std::string slot = editor().targetSlot();
				   const Xml::Layout* layout = editor().draft(slot);
				   Xml::Services::askNumber(
					   {.header = _("customise.columns"),
						.min = static_cast<float>(Xml::Layout::MIN_CELLS),
						.max = static_cast<float>(Xml::Layout::MAX_CELLS),
						.value = layout ? std::optional(static_cast<float>(layout->columns)) : std::nullopt,
						.onConfirm = [this, slot](float value)
						{
							if (const Xml::Layout* current = editor().draft(slot))
							{
								editor().setGrid(slot, static_cast<int32_t>(std::lround(value)), current->rows);
							}
							showGrid();
						}});
			   });
		action("customise.rows",
			   "Ask for the grid's number of rows on the number pad",
			   [this](const Xml::ActionArgs&)
			   {
				   const std::string slot = editor().targetSlot();
				   const Xml::Layout* layout = editor().draft(slot);
				   Xml::Services::askNumber(
					   {.header = _("customise.rows"),
						.min = static_cast<float>(Xml::Layout::MIN_CELLS),
						.max = static_cast<float>(Xml::Layout::MAX_CELLS),
						.value = layout ? std::optional(static_cast<float>(layout->rows)) : std::nullopt,
						.onConfirm = [this, slot](float value)
						{
							if (const Xml::Layout* current = editor().draft(slot))
							{
								editor().setGrid(slot, current->columns, static_cast<int32_t>(std::lround(value)));
							}
							showGrid();
						}});
			   });
		action("customise.cancel",
			   "Stop customising and throw the changes away",
			   [](const Xml::ActionArgs&) { editor().cancel(); });
		action("customise.done", "Stop customising and keep the changes", [this](const Xml::ActionArgs&) { finish(); });
	}

	CustomisePresenter::~CustomisePresenter()
	{
		editor().removeListener(this);
	}

	void CustomisePresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("dialog_open", m_dialogOpen, "1 while any of the dialogs below is open, else 0");
		f.list("catalogue", m_catalogue, "The widgets that can be added where the user chose, by category");
		f.field("catalogue_open", m_catalogueOpen, "1 while the widget pool is open");
		f.list("settings", m_settings, "The selected widget's settings");
		f.field("settings_open", m_settingsOpen, "1 while the selected widget's settings are open");
		f.list("options", m_options, "The values the setting being changed can take");
		f.field("options_open", m_optionsOpen, "1 while they are shown");
		f.field("options_title", m_optionsTitle, "The name of the setting they are for");
		f.field("keyboard", m_keyboard, "1 while the keyboard is up for a text setting, so the settings move above it");
		f.field("grid_open", m_gridOpen, "1 while the grid's size is shown");
		f.field("columns", m_columns, "The grid's number of columns");
		f.field("rows", m_rows, "The grid's number of rows");
	}

	void CustomisePresenter::registerActions()
	{
		Xml::Actions::addGlobal("customise.start",
								"Go to the home screen and start customising it: its widgets can be moved, resized, "
								"added and removed, and their settings changed",
								[](const Xml::ActionArgs&)
								{
									home();
									editor().begin();
								});
		Xml::Actions::addGlobal("customise.open_catalogue",
								"While customising, open the widget pool to add a widget where the user chose",
								[](const Xml::ActionArgs&)
								{
									if (auto* customise = sharedCustomise())
									{
										customise->openCatalogue();
									}
								});
	}

	void CustomisePresenter::onEditorChanged()
	{
		if (!editor().isEditing())
		{
			closeDialogs();
		}
	}

	void CustomisePresenter::openCatalogue()
	{
		if (!editor().isEditing())
		{
			return;
		}
		// The widgets that fit, by category, each in name order
		const std::string slot = editor().targetSlot();
		std::map<std::string, std::vector<const Xml::WidgetDef*>> byCategory;
		for (const auto& [id, widget] : Xml::WidgetCatalogue::all())
		{
			if (widget.fits(slot))
			{
				byCategory[shown(widget.categoryTag, widget.category.empty() ? "?" : widget.category)].push_back(
					&widget);
			}
		}
		std::vector<std::pair<std::string, std::vector<const Xml::WidgetDef*>>> categories(byCategory.begin(),
																						   byCategory.end());
		m_catalogue.sync(categories.size(),
						 [&categories](CategoryRow& row, size_t index)
						 {
							 auto& [title, widgets] = categories[index];
							 std::sort(widgets.begin(),
									   widgets.end(),
									   [](const Xml::WidgetDef* a, const Xml::WidgetDef* b)
									   { return shown(a->titleTag, a->title) < shown(b->titleTag, b->title); });
							 row.title.set(title);
							 row.widgets.sync(widgets.size(),
											  [&widgets](CatalogueRow& item, size_t i)
											  {
												  const Xml::WidgetDef& widget = *widgets[i];
												  item.id.set(widget.id);
												  item.title.set(shown(widget.titleTag, widget.title));
												  item.description.set(
													  shown(widget.descriptionTag, widget.description));
												  item.icon.set(widget.icon);
											  });
						 });
		setOpen(m_catalogueOpen, 1);
	}

	void CustomisePresenter::showSettings()
	{
		const Xml::WidgetDef* widget = selectedWidget();
		const size_t count = widget ? widget->settings.size() : 0;
		m_settings.sync(count,
						[widget](SettingRow& row, size_t index)
						{
							const Xml::SettingDef& setting = widget->settings[index];
							const nlohmann::json value = currentValue(setting);
							row.name.set(setting.name);
							row.title.set(shown(setting.titleTag, setting.title));
							row.kind.set(Xml::WidgetCatalogue::kindName(setting.kind));
							row.control.set(setting.kind == Xml::SettingKind::Bool	 ? "switch"
											: setting.kind == Xml::SettingKind::Text ? "text"
																					 : "button");
							row.value.set(shownValue(setting, value));
							row.on.set(value.is_boolean() && value.get<bool>() ? 1 : 0);
						});
	}

	void CustomisePresenter::editSetting(const SettingRow& row)
	{
		const Xml::WidgetDef* widget = selectedWidget();
		const Xml::SettingDef* setting = widget ? widget->setting(row.name.get()) : nullptr;
		if (setting == nullptr)
		{
			return;
		}
		m_choosing = setting->name;
		const std::string title = shown(setting->titleTag, setting->title);
		const nlohmann::json value = currentValue(*setting);
		switch (setting->kind)
		{
		case Xml::SettingKind::Int:
		{
			const std::string name = setting->name;
			Xml::Services::askNumber({.header = title,
									  .min = static_cast<float>(setting->min),
									  .max = static_cast<float>(setting->max),
									  .value = static_cast<float>(value.get<int32_t>()),
									  .onConfirm = [this, name](float typed)
									  { setSetting(name, static_cast<int32_t>(std::lround(typed))); }});
			break;
		}
		case Xml::SettingKind::Bool:
			setSetting(setting->name, !(value.is_boolean() && value.get<bool>()));
			break;
		case Xml::SettingKind::Text:
			break; // Typed in its row
		case Xml::SettingKind::Choice:
		case Xml::SettingKind::Row:
		{
			const std::string current = Xml::SettingDef::toProp(value);
			const auto choices = setting->choices();
			m_options.sync(choices.size(),
						   [&](OptionRow& option, size_t index)
						   {
							   option.value.set(choices[index].value);
							   option.title.set(shown(choices[index].titleTag, choices[index].title));
							   option.chosen.set(choices[index].value == current ? 1 : 0);
						   });
			m_optionsTitle.set(title);
			setOpen(m_optionsOpen, 1);
			break;
		}
		}
	}

	void CustomisePresenter::setSetting(const std::string& name, const nlohmann::json& value)
	{
		editor().setSetting(name, value);
		showSettings();
	}

	void CustomisePresenter::showGrid()
	{
		const Xml::Layout* layout = editor().draft(editor().targetSlot());
		m_columns.set(layout ? std::to_string(layout->columns) : "");
		m_rows.set(layout ? std::to_string(layout->rows) : "");
	}

	void CustomisePresenter::closeDialogs()
	{
		m_keyboard.set(0);
		setOpen(m_catalogueOpen, 0);
		setOpen(m_settingsOpen, 0);
		setOpen(m_optionsOpen, 0);
		setOpen(m_gridOpen, 0);
	}

	void CustomisePresenter::setOpen(Xml::IntSubject& dialog, bool open)
	{
		dialog.set(open ? 1 : 0);
		const bool any = m_catalogueOpen.get() || m_settingsOpen.get() || m_optionsOpen.get() || m_gridOpen.get();
		m_dialogOpen.set(any ? 1 : 0);
	}

	void CustomisePresenter::finish()
	{
		std::string error;
		if (!editor().done(error))
		{
			Model::get().post<EventType::Response>(ResponseType::WARNING,
												   fmt::format("{:s}: {:s}", _("customise.not_saved"), error));
		}
	}
} // namespace UI
