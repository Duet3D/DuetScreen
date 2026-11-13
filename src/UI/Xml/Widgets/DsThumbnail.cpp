/*
 * DsThumbnail.cpp
 */

#include "DsThumbnail.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "src/xml/parsers/lv_xml_image_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsThumbnail::DsThumbnail(const std::string& name, lv_obj_t* parent)
		: DsThumbnail(name, LvObjRef(parent))
	{
	}

	DsThumbnail::DsThumbnail(const std::string& name, LvObjRef&& parent)
		: LvImage(name, parent)
		, m_parent(parent.getRootPtr())
	{
		lv_obj_add_event_cb(m_parent, parentChangedCb, LV_EVENT_CHILD_CHANGED, this);
	}

	DsThumbnail::~DsThumbnail()
	{
		lv_obj_remove_event_cb_with_user_data(m_parent, parentChangedCb, this);
	}

	void DsThumbnail::registerXml()
	{
		Widgets::add(
			{.name = "ds_thumbnail",
			 .extends = "lv_image",
			 .description = "An image as tall as its parent's content and as wide as its aspect ratio needs, so a "
							"thumbnail beside some text is as large as it can be without making the row taller. With "
							"no image it takes no width.",
			 .example = R"(<ds_thumbnail bind_src="thumbnail" height="100%"/>)",
			 .attributes = {{"bind_src", "Follow a text subject holding an image path; empty shows no image"}}},
			xmlCreate,
			xmlApply);
	}

	void DsThumbnail::setImage(std::string_view path)
	{
		m_path = path;
		setSrc(m_path.empty() ? nullptr : m_path.c_str());
		lv_obj_update_layout(m_parent);
	}

	void* DsThumbnail::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* thumbnail = new DsThumbnail(attrOr(attrs, "name", "thumbnail"), parentOf(state));
		deleteWith(thumbnail->getRootPtr(), thumbnail);
		return thumbnail->getRootPtr();
	}

	void DsThumbnail::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_image_apply(state, attrs);
		auto& thumbnail = static_cast<DsThumbnail&>(*LvObj::fromPtr(itemOf(state)));
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_src"))
		{
			lv_subject_add_observer_obj(subject, srcObserverCb, thumbnail.getRootPtr(), &thumbnail);
		}
	}

	void DsThumbnail::srcObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsThumbnail*>(lv_observer_get_user_data(observer))->setImage(lv_subject_get_string(subject));
	}

	void DsThumbnail::parentChangedCb(lv_event_t* e)
	{
		auto* thumbnail = static_cast<DsThumbnail*>(lv_event_get_user_data(e));
		if (thumbnail->getRootPtr() != nullptr)
		{
			thumbnail->fit();
		}
	}

	void DsThumbnail::fit()
	{
		const int32_t srcWidth = getSrc() ? getSrcWidth() : 0;
		const int32_t srcHeight = getSrc() ? getSrcHeight() : 0;
		if (srcWidth <= 0 || srcHeight <= 0)
		{
			setWidth(0);
			return;
		}
		// The parent's content height, not this image's: that is only updated after this layout pass
		const int32_t height = lv_obj_calc_content_height(m_parent);
		setWidth(height * (100 * srcWidth / srcHeight) / 100);
	}
} // namespace UI::Xml
