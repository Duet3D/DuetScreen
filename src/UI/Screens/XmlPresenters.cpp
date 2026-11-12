/*
 * XmlPresenters.cpp
 */

#include "XmlPresenters.h"
#include "Fan/FanPresenter.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	void registerXmlPresenters()
	{
		Xml::Presenters::add<FanPresenter>("fans");
	}
} // namespace UI
