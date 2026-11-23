/*
 * DsDialogFooter.h
 *
 * <ds_dialog_footer>: a step's own buttons, written last in its content. While the step shows, the dialog surface
 * (DialogHost.h) keeps them at the bottom of the card, below the content, which scrolls on its own; they go back to
 * their place in the content when the step does. Actions raised in them still go to the view the content is written
 * in.
 *
 *     <ds_dialog name="save_dialog" ...>
 *         …the content…
 *         <ds_dialog_footer>
 *             <ds_button name="cancel" .../>
 *             <ds_button name="ok" ...><style name="action_btn"/></ds_button>
 *         </ds_dialog_footer>
 *     </ds_dialog>
 */

#pragma once

#include "helix_xml.h"

namespace UI::Xml::DsDialogFooter
{
	/** Register `<ds_dialog_footer>` with the XML engine */
	void registerXml();

	/** The footer written in `content` (one of its children), or nullptr */
	lv_obj_t* in(lv_obj_t* content);
} // namespace UI::Xml::DsDialogFooter
