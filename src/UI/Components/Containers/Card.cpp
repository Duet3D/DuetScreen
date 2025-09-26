/*
 * Card.cpp
 *
 *  Created on: 2025-08-07
 *      Author: Andy Everitt
 */

#include "Card.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	Card::Card(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{

		init();
	}

	Card::Card(const std::string& name, LvObj& parent, layout_t layout)
		: LvContainer(name, parent, layout)
	{
		init();
	}

	void Card::init()
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().card);
	}
} // namespace UI
