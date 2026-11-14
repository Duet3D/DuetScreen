/*
 * ObjectModelSource.cpp
 */

#include "ObjectModelSource.h"
#include "ObjectModel/Tree/Tree.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Binding/Globals.h"

namespace UI::Xml
{
	ObjectModelSource::ObjectModelSource()
		: DataSource("om")
	{
		Globals::setResolver(&m_bindings);
		// Called on the decoder's thread: the flush happens on the UI's, through the event
		m_treeListener = OM::Tree::get().addListener([] { Model::get().post<EventType::ObjectModelChanged>(); });
	}

	ObjectModelSource::~ObjectModelSource()
	{
		OM::Tree::get().removeListener(m_treeListener);
		Globals::setResolver(nullptr);
	}

	void ObjectModelSource::onInit()
	{
		registerEventListener<EventType::ObjectModelChanged>(this, &ObjectModelSource::changed);
	}

	void ObjectModelSource::update()
	{
		m_bindings.refreshAll();
	}

	void ObjectModelSource::clear()
	{
		m_bindings.flush();
	}

	void ObjectModelSource::changed()
	{
		m_bindings.flush();
	}
} // namespace UI::Xml
