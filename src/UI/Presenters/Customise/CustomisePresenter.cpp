/*
 * CustomisePresenter.cpp
 */

#include "CustomisePresenter.h"
#include "Debug.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Layout/LayoutLibrary.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Layout/WidgetCatalogue.h"
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
			case Xml::SettingKind::Action:
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

	CustomisePresenter* CustomisePresenter::shared()
	{
		auto* view = findView<Xml::XmlView>("customise");
		auto* presenter = view != nullptr ? dynamic_cast<CustomisePresenter*>(view->getPresenter()) : nullptr;
		if (presenter == nullptr)
		{
			LOG_WARN("There is no customise toolbar (a view called 'customise' showing customise.xml)");
		}
		return presenter;
	}

	void CatalogueRow::fields(Xml::FieldVisitor& f)
	{
		f.field("id", id, "The widget's id");
		f.field("title", title, "Its name, translated");
		f.field("description", description, "What it shows, translated");
		f.field("icon", icon, "Its icon, e.g. \"fan.png\"; empty for none");
	}

	void SavedRow::fields(Xml::FieldVisitor& f)
	{
		f.field("name", name, "The saved layout's name");
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

	CustomisePresenter::CustomisePresenter()
		: XmlPresenter("customise")
	{
		editor().addListener(this);

		action("customise.add",
			   "Add the widget of the widget pool's row where it was asked for, and close the pool",
			   [this](const Xml::ActionArgs& args)
			   {
				   m_catalogueOpen.set(0);
				   if (const auto* row = args.row<CatalogueRow>())
				   {
					   editor().add(std::string(row->id.get()));
				   }
			   });
		action("customise.configure",
			   "Show the selected widget's settings",
			   [this](const Xml::ActionArgs&)
			   {
				   showSettings();
				   m_openedFor = editor().selection();
				   m_settingsOpen.set(editor().canConfigure() ? 1 : 0);
			   });
		action("customise.edit_setting",
			   "Change the setting of the row: on a number pad or a choice of its values, the settings' next step",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<SettingRow>())
				   {
					   editSetting(*row, args.target);
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
		action("customise.save",
			   "Ask for a name, to save every slot's layout as it is now under it",
			   [this](const Xml::ActionArgs&) { askSaveName(); });
		action("customise.save_name",
			   "The name being typed changed (the text area's text)",
			   [this](const Xml::ActionArgs& args)
			   {
				   m_saveName.set(args.text());
				   m_saveProblem.set("");
			   });
		action("customise.save_ok", "Save under the name typed", [this](const Xml::ActionArgs&) { saveAs(); });
		action("customise.load",
			   "List the saved layouts, to load one",
			   [this](const Xml::ActionArgs&)
			   {
				   showSaved();
				   m_loadOpen.set(1);
			   });
		action("customise.load_saved",
			   "Show the saved layout of the row, as one change, and close the list",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<SavedRow>())
				   {
					   loadSaved(std::string(row->name.get()));
				   }
			   });
		action("customise.ask_delete",
			   "Ask whether to delete the saved layout of the row",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<SavedRow>())
				   {
					   const std::string name(row->name.get());
					   // The question goes with the list's row, and so with this
					   Xml::Prompts::askQuestion({.title = _("customise.delete_title"),
												  .text = _("customise.delete_text", name),
												  .onYes =
													  [this, name]
												  {
													  Xml::LayoutLibrary::remove(name);
													  showSaved();
												  },
												  .asker = args.target});
				   }
			   });
		action("customise.remove",
			   "Remove the selected widget",
			   [](const Xml::ActionArgs&) { editor().removeSelected(); });
		action("customise.undo", "Undo the last change", [](const Xml::ActionArgs&) { editor().undo(); });
		action("customise.reset",
			   "Put the selected widget's slot (else the dashboard) back as it was before any customising",
			   [](const Xml::ActionArgs&) { editor().reset(editor().targetSlot()); });
		action("customise.snap",
			   "Turn snapping of dragged widgets on or off, in the selected widget's slot (else the dashboard)",
			   [](const Xml::ActionArgs&)
			   {
				   const std::string slot = editor().targetSlot();
				   if (const Xml::Layout* layout = editor().draft(slot))
				   {
					   editor().setSnap(slot, !layout->snap);
				   }
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
		f.list("catalogue", m_catalogue, "The widgets that can be added where the user chose, by category");
		f.field("catalogue_open", m_catalogueOpen, "1 while the widget pool is open");
		f.list("settings", m_settings, "The selected widget's settings");
		f.field("settings_open", m_settingsOpen, "1 while the selected widget's settings are open");
		f.field("can_save", m_canSave, "1 if layouts can be saved (not in safe mode), else 0");
		f.field("save_open", m_saveOpen, "1 while asking for a name to save the layouts as");
		f.field("save_name", m_saveName, "The name typed so far");
		f.field("save_problem", m_saveProblem, "Why the name typed can't be used, translated; empty if it can");
		f.field("load_open", m_loadOpen, "1 while the saved layouts are listed");
		f.list("saved", m_saved, "The saved layouts, by name");
		f.field("has_saved", m_hasSaved, "1 if there are any saved layouts, else 0");
	}

	void CustomisePresenter::onEditorChanged()
	{
		if (!editor().isEditing())
		{
			closeDialogs();
		}
		// The pool and the settings are for what was selected: selecting something else (a widget tapped beside the
		// dialog, in the sidebar or status bar) closes them, as tapping outside a dialog does
		else if ((m_settingsOpen.get() != 0 || m_catalogueOpen.get() != 0) && editor().selection() != m_openedFor)
		{
			m_settingsOpen.set(0);
			m_catalogueOpen.set(0);
		}
		m_canSave.set(Xml::LayoutLibrary::canSave() ? 1 : 0);
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
		m_openedFor = editor().selection();
		m_catalogueOpen.set(1);
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

	void CustomisePresenter::editSetting(const SettingRow& row, lv_obj_t* asker)
	{
		const Xml::WidgetDef* widget = selectedWidget();
		const Xml::SettingDef* setting = widget ? widget->setting(row.name.get()) : nullptr;
		if (setting == nullptr)
		{
			return;
		}
		const std::string name = setting->name;
		const std::string title = shown(setting->titleTag, setting->title);
		const nlohmann::json value = currentValue(*setting);
		switch (setting->kind)
		{
		case Xml::SettingKind::Int:
			// The prompts go with the setting's row, and so with this
			Xml::Prompts::askNumber(
				{.header = title,
				 .min = static_cast<float>(setting->min),
				 .max = static_cast<float>(setting->max),
				 .value = static_cast<float>(value.get<int32_t>()),
				 .onConfirm = [this, name](float typed) { setSetting(name, static_cast<int32_t>(std::lround(typed))); },
				 .asker = asker});
			break;
		case Xml::SettingKind::Bool:
			setSetting(setting->name, !(value.is_boolean() && value.get<bool>()));
			break;
		case Xml::SettingKind::Text:
			break; // Typed in its row
		case Xml::SettingKind::Choice:
		case Xml::SettingKind::Row:
		case Xml::SettingKind::Action:
		{
			std::vector<Xml::Prompts::Choice> choices;
			for (const auto& option : setting->choices())
			{
				choices.push_back({.value = option.value, .title = shown(option.titleTag, option.title)});
			}
			Xml::Prompts::askChoice({.title = title,
									 .choices = std::move(choices),
									 .chosen = Xml::SettingDef::toProp(value),
									 .onChoose = [this, name](const std::string& chosen) { setSetting(name, chosen); },
									 .asker = asker});
			break;
		}
		}
	}

	void CustomisePresenter::setSetting(const std::string& name, const nlohmann::json& value)
	{
		editor().setSetting(name, value);
		showSettings();
	}

	void CustomisePresenter::closeDialogs()
	{
		m_saveOpen.set(0);
		m_loadOpen.set(0);
		m_catalogueOpen.set(0);
		m_settingsOpen.set(0);
	}

	void CustomisePresenter::askSaveName()
	{
		m_saveName.set("");
		m_saveProblem.set("");
		m_saveOpen.set(1);
	}

	void CustomisePresenter::saveAs()
	{
		const std::string name(m_saveName.get());
		if (!Xml::LayoutLibrary::isValidName(name))
		{
			m_saveProblem.set(_("customise.bad_name", Xml::LayoutLibrary::MAX_NAME));
			return;
		}
		std::string error;
		if (!Xml::LayoutLibrary::save(name, editor().drafts(), error))
		{
			warn(fmt::format("{:s}: {:s}", _("customise.not_saved"), error));
		}
		m_saveOpen.set(0);
	}

	void CustomisePresenter::showSaved()
	{
		const std::vector<std::string> names = Xml::LayoutLibrary::names();
		m_saved.sync(names.size(), [&names](SavedRow& row, size_t index) { row.name.set(names[index]); });
		m_hasSaved.set(names.empty() ? 0 : 1);
		m_canSave.set(Xml::LayoutLibrary::canSave() ? 1 : 0);
	}

	void CustomisePresenter::loadSaved(const std::string& name)
	{
		m_loadOpen.set(0);
		std::string error;
		if (auto layouts = Xml::LayoutLibrary::load(name, error))
		{
			editor().apply(*layouts);
		}
		else
		{
			warn(fmt::format("{:s}: {:s}", _("customise.not_loaded"), error));
		}
	}

	void CustomisePresenter::warn(const std::string& message)
	{
		Model::get().post<EventType::Response>(ResponseType::WARNING, message);
	}

	void CustomisePresenter::finish()
	{
		std::string error;
		if (!editor().done(error))
		{
			warn(fmt::format("{:s}: {:s}", _("customise.not_saved"), error));
		}
	}
} // namespace UI
