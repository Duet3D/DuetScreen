/*
 * XmlView.cpp
 */

#include "XmlView.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "XmlActions.h"
#include "XmlUi.h"
#include "helix_xml.h"

namespace UI::Xml
{
	namespace
	{
		std::shared_ptr<XmlPresenter> createPresenterFor(const std::string& component)
		{
			const ComponentInfo* info = findComponent(component);
			if (info == nullptr || info->presenter.empty())
			{
				return nullptr;
			}
			auto presenter = Presenters::create(info->presenter);
			if (!presenter)
			{
				LOG_ERROR("Component '{:s}' names unknown presenter '{:s}'", component, info->presenter);
			}
			return presenter;
		}
	} // namespace

	detail::XmlViewState::XmlViewState(std::string componentName, std::string name)
		: component(std::move(componentName))
		, objName(std::move(name))
		, presenter(createPresenterFor(component))
	{
	}

	lv_xml_subject_ctx_t* detail::XmlViewState::context() const
	{
		return presenter ? presenter->getContext() : emptyContext.get();
	}

	XmlView::XmlView(const std::string& name, LvObj& parent, std::string componentName)
		: XmlViewState(std::move(componentName), name)
		, LvObj([this](lv_obj_t* p) { return build(p, *this); }, name, parent.getRootPtr())
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
		if (getRootPtr())
		{
			Actions::unregisterScope(getRootPtr());
		}
		// ~LvObj deletes the widgets next; the presenter and its subjects outlive them
	}

	lv_obj_t* XmlView::build(lv_obj_t* parent, detail::XmlViewState& state)
	{
		ZoneScoped;
		auto* obj =
			static_cast<lv_obj_t*>(lv_xml_create_with_ctx(parent, state.component.c_str(), nullptr, state.context()));
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
		Actions::registerScope(getRootPtr(), presenter ? &presenter->getActions() : nullptr);
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

		// A replaced definition may name a different presenter
		const ComponentInfo* info = findComponent(component);
		const std::string wanted = info ? info->presenter : "";
		if (wanted != (presenter ? std::string(presenter->getName()) : ""))
		{
			if (presenter)
			{
				presenter->deactivate();
				Model::get().unbind(presenter);
			}
			presenter = createPresenterFor(component);
		}

		lv_obj_t* parent = lv_obj_get_parent(old);
		const int32_t index = lv_obj_get_index(old);
		const bool hidden = lv_obj_has_flag(old, LV_OBJ_FLAG_HIDDEN);

		lv_obj_t* fresh = build(parent, *this);
		lv_obj_move_to_index(fresh, index);
		lv_obj_set_flag(fresh, LV_OBJ_FLAG_HIDDEN, hidden);

		// Deleting the old root nulls m_root (LvObj registers lv_obj_null_on_delete), then adopt the new one
		Actions::unregisterScope(old);
		lv_obj_delete(old);
		m_root = fresh;
		lv_obj_set_name(m_root, objName.c_str());
		lv_obj_set_user_data(m_root, static_cast<LvObj*>(this));
		lv_obj_null_on_delete(&m_root);
		registerActions();

		if (!hidden && presenter)
		{
			presenter->activate();
			Model::get().bind(presenter);
		}
	}

	void XmlView::showInner()
	{
		if (presenter)
		{
			presenter->activate();
			Model::get().bind(presenter);
		}
		LvObj::showInner();
	}

	void XmlView::hideInner()
	{
		if (presenter)
		{
			presenter->deactivate();
			Model::get().unbind(presenter);
		}
		LvObj::hideInner();
	}
} // namespace UI::Xml
