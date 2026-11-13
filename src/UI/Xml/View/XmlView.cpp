/*
 * XmlView.cpp
 */

#include "XmlView.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "helix_xml.h"

namespace UI::Xml
{
	namespace
	{
		/** The presenter name in the loaded definition of `component`; empty if none */
		std::string presenterNameOf(const std::string& component)
		{
			const ComponentInfo* info = findComponent(component);
			return info ? info->presenter : std::string();
		}

		/** A new presenter for `component`, or nullptr if it names none or an unknown one */
		std::shared_ptr<XmlPresenter> createPresenterFor(const std::string& component)
		{
			const std::string name = presenterNameOf(component);
			if (name.empty())
			{
				return nullptr;
			}
			auto presenter = Presenters::create(name);
			if (!presenter)
			{
				LOG_ERROR("Component '{:s}' names unknown presenter '{:s}'", component, name);
			}
			return presenter;
		}
	} // namespace

	detail::XmlViewState::XmlViewState(std::string componentName, std::string name, std::shared_ptr<XmlPresenter> given)
		: component(std::move(componentName))
		, objName(std::move(name))
		, presenter(std::move(given))
		, presenterGiven(presenter != nullptr)
	{
		prepare();
	}

	void detail::XmlViewState::prepare()
	{
		if (!presenterGiven)
		{
			const std::string wanted = presenterNameOf(component);
			const std::string current = presenter ? std::string(presenter->getName()) : std::string();
			if (!presenter || wanted != current)
			{
				presenter = createPresenterFor(component);
			}
		}

		// A screen is self-contained: its names are its presenter's, then the globals
		scope = std::make_unique<Scope>(nullptr);
		if (presenter)
		{
			ScopeBinder binder(*scope);
			presenter->fields(binder);
		}
	}

	XmlView::XmlView(const std::string& name, LvObj& parent, std::string componentName)
		: XmlView(name, parent.getRootPtr(), std::move(componentName))
	{
	}

	XmlView::XmlView(const std::string& name,
					 LvObj& parent,
					 std::string componentName,
					 std::shared_ptr<XmlPresenter> givenPresenter)
		: XmlView(name, parent.getRootPtr(), std::move(componentName), std::move(givenPresenter))
	{
	}

	XmlView::XmlView(const std::string& name, lv_obj_t* parent, std::string componentName)
		: XmlView(name, parent, std::move(componentName), nullptr)
	{
	}

	XmlView::XmlView(const std::string& name,
					 lv_obj_t* parent,
					 std::string componentName,
					 std::shared_ptr<XmlPresenter> givenPresenter)
		: XmlViewState(std::move(componentName), name, std::move(givenPresenter))
		, LvObj([this](lv_obj_t* p) { return build(p, *this); }, name, parent)
	{
		ZoneScoped;
		registerActions();
		addView(this);
	}

	XmlView::~XmlView()
	{
		ZoneScoped;
		UI_LOCK();
		removeView(this);
		if (presenter)
		{
			Model::get().unbind(presenter);
		}
		// ~LvObj deletes the widgets next; the presenter and scope (members of the base) outlive them
	}

	lv_obj_t* XmlView::build(lv_obj_t* parent, detail::XmlViewState& state)
	{
		ZoneScoped;
		auto* obj = static_cast<lv_obj_t*>(
			lv_xml_create_with_ctx(parent, state.component.c_str(), nullptr, state.scope->context()));
		if (obj != nullptr)
		{
			return obj;
		}

		// Never leave a hole: say what's missing where the view should be
		LOG_ERROR("Couldn't build UI component '{:s}'", state.component);
		obj = lv_obj_create(parent);
		lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
		lv_obj_t* label = lv_label_create(obj);
		lv_label_set_text_fmt(label, "UI component '%s' couldn't be loaded", state.component.c_str());
		lv_obj_center(label);
		return obj;
	}

	void XmlView::registerActions()
	{
		Actions::setHandlers(getRootPtr(), presenter ? &presenter->getActions() : nullptr);
	}

	void XmlView::rebuild()
	{
		ZoneScoped;
		UI_LOCK();
		lv_obj_t* old = getRootPtr();
		if (old == nullptr)
		{
			return;
		}

		lv_obj_t* parent = lv_obj_get_parent(old);
		const int32_t index = lv_obj_get_index(old);
		const bool hidden = lv_obj_has_flag(old, LV_OBJ_FLAG_HIDDEN);

		// The old widgets go first: they may be bound to the scope (and presenter) about to be replaced.
		// Deleting the root nulls m_root (LvObj registers lv_obj_null_on_delete).
		disconnectPresenter();
		lv_obj_delete(old);
		prepare();

		m_root = build(parent, *this);
		lv_obj_move_to_index(m_root, index);
		lv_obj_set_flag(m_root, LV_OBJ_FLAG_HIDDEN, hidden);
		lv_obj_set_name(m_root, objName.c_str());
		lv_obj_set_user_data(m_root, static_cast<LvObj*>(this));
		lv_obj_null_on_delete(&m_root);
		registerActions();

		if (!hidden)
		{
			connectPresenter();
		}
	}

	void XmlView::showInner()
	{
		connectPresenter();
		LvObj::showInner();
	}

	void XmlView::hideInner()
	{
		disconnectPresenter();
		LvObj::hideInner();
	}

	void XmlView::connectPresenter()
	{
		if (presenter)
		{
			presenter->activate();
			Model::get().bind(presenter);
		}
	}

	void XmlView::disconnectPresenter()
	{
		if (presenter)
		{
			presenter->deactivate();
			Model::get().unbind(presenter);
		}
	}
} // namespace UI::Xml
