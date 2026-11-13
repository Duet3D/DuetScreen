/*
 * CustomiseSource.cpp
 */

#include "CustomiseSource.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Layout/WidgetCatalogue.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI::Xml
{
	CustomiseSource::CustomiseSource()
		: DataSource("customise")
	{
		LayoutEditor::get().addListener(this);
	}

	CustomiseSource::~CustomiseSource()
	{
		LayoutEditor::get().removeListener(this);
	}

	void CustomiseSource::fields(FieldVisitor& f)
	{
		f.field("customise.editing", m_editing, "1 while the user is customising the screen, else 0");
		f.field("customise.selected",
				m_selected,
				"The selected widget, as \"slot/id\" (e.g. \"dashboard/tools\"), or empty if none is");
		f.field("customise.has_item", m_hasItem, "1 if a widget is selected, else 0");
		f.field("customise.can_remove", m_canRemove, "1 if the selected widget can be removed (it isn't locked)");
		f.field("customise.can_configure", m_canConfigure, "1 if the selected widget has settings the user can change");
		f.field("customise.can_undo", m_canUndo, "1 if there is a change to undo");
		f.field("customise.is_free", m_isFree, "1 if new widgets go into a free layout, whose widgets can snap");
		f.field("customise.snap", m_snap, "1 if dragged widgets snap to the edges near them, in that free layout");
		f.field("customise.title", m_title, "The selected widget's name, translated; empty if none is selected");
	}

	void CustomiseSource::update()
	{
		const LayoutEditor& editor = LayoutEditor::get();
		const LayoutEditor::Selection& selection = editor.selection();
		const LayoutItem* item = editor.selectedItem();
		const WidgetDef* widget = item ? WidgetCatalogue::find(item->widget) : nullptr;
		const Layout* target = editor.draft(editor.targetSlot());

		m_editing.set(editor.isEditing() ? 1 : 0);
		m_selected.set(item ? selectedKey(selection.slot, selection.item) : "");
		m_hasItem.set(item ? 1 : 0);
		m_canRemove.set(editor.canRemove() ? 1 : 0);
		m_canConfigure.set(editor.canConfigure() ? 1 : 0);
		m_canUndo.set(editor.canUndo() ? 1 : 0);
		m_isFree.set(target && target->kind == LayoutKind::Free ? 1 : 0);
		m_snap.set(target && target->kind == LayoutKind::Free && target->snap ? 1 : 0);
		if (widget == nullptr)
		{
			m_title.set(item ? item->widget : "");
		}
		else
		{
			m_title.set(widget->titleTag.empty() ? widget->title : std::string(_(widget->titleTag.c_str())));
		}
	}

	std::string CustomiseSource::selectedKey(std::string_view slot, std::string_view id)
	{
		return fmt::format("{:s}/{:s}", slot, id);
	}
} // namespace UI::Xml
