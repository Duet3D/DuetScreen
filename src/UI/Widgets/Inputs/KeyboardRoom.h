/*
 * KeyboardRoom.h
 *
 * Room for the on-screen keyboard, so it never covers the text box it types into. When a `<ds_textarea>` brings the
 * keyboard up, what the box is in makes room above it:
 *
 *  - in a dialog, the card moves to the top and keeps above the keyboard, scrolling the box into view
 *    (DialogHost::makeRoom());
 *  - on a page, the nearest scrolling container around the box that a keyboard floating over it covers gets that
 *    much more padding at its bottom, and the box is scrolled into view; a keyboard laid out in the container (the
 *    hardware test's) takes its room already.
 *
 * Nothing in XML asks for it. When the keyboard goes, the room is given back.
 */

#pragma once

#include "helix_xml.h"

namespace UI::Xml::KeyboardRoom
{
	/** `keyboard` has come up for `textarea`: make room for it */
	void make(lv_obj_t* keyboard, lv_obj_t* textarea);

	/** The keyboard has gone: give the room back */
	void giveBack();

	/**
	 * Put the keyboard away if the box it types into is `obj` or inside it: `obj` is going out of sight, e.g. the
	 * dialog step it is in closes, so the edit ends
	 */
	void putAwayFrom(lv_obj_t* obj);
} // namespace UI::Xml::KeyboardRoom
