/*
 * Presenters.cpp
 */

#include "Presenters.h"
#include "FansPresenter.h"

namespace UI::Xml
{
	void registerBuiltinPresenters()
	{
		Presenters::add<FansPresenter>("fans");
	}
} // namespace UI::Xml
