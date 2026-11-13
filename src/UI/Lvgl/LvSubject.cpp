/*
 * LvSubject.cpp
 *
 *  Created on: 2026-01-13
 *      Author: Andy Everitt
 */

#include "LvSubject.h"
#include "Debug.h"

namespace UI
{
#if LV_USE_OBSERVER
	LvSubject::LvSubject() {}

	LvSubject::~LvSubject()
	{
		ZoneScoped;
		UI_LOCK();
		if (m_initialized)
		{
			deinit();
		}
	}

	void LvSubject::initInt(int32_t value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_init_int(&m_subject, value);
		m_initialized = true;
	}

	void LvSubject::setInt(int32_t value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_set_int(&m_subject, value);
	}

	int32_t LvSubject::getInt() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_int(const_cast<lv_subject_t*>(&m_subject));
	}

	int32_t LvSubject::getPreviousInt() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_previous_int(const_cast<lv_subject_t*>(&m_subject));
	}

	void LvSubject::setMinInt(int32_t min_value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_set_min_value_int(&m_subject, min_value);
	}

	void LvSubject::setMaxInt(int32_t max_value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_set_max_value_int(&m_subject, max_value);
	}

#  if LV_USE_FLOAT

	void LvSubject::initFloat(float value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_init_float(&m_subject, value);
		m_initialized = true;
	}

	void LvSubject::setFloat(float value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_set_float(&m_subject, value);
	}

	float LvSubject::getFloat() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_float(const_cast<lv_subject_t*>(&m_subject));
	}

	float LvSubject::getPreviousFloat() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_previous_float(const_cast<lv_subject_t*>(&m_subject));
	}

	void LvSubject::setMinFloat(float min_value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_set_min_value_float(&m_subject, min_value);
	}

	void LvSubject::setMaxFloat(float max_value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_set_max_value_float(&m_subject, max_value);
	}

#  endif

	void LvSubject::initString(char* buf, char* prev_buf, size_t size, const char* value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_init_string(&m_subject, buf, prev_buf, size, value);
		m_initialized = true;
	}

	void LvSubject::copyString(const char* value)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_copy_string(&m_subject, value);
	}

	std::string_view LvSubject::getString() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_string(const_cast<lv_subject_t*>(&m_subject));
	}

	std::string_view LvSubject::getPreviousString() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_previous_string(const_cast<lv_subject_t*>(&m_subject));
	}

	void LvSubject::initPointer(void* pointer)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_init_pointer(&m_subject, pointer);
		m_initialized = true;
	}

	void LvSubject::setPointer(void* pointer)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_set_pointer(&m_subject, pointer);
	}

	const void* LvSubject::getPointer() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_pointer(const_cast<lv_subject_t*>(&m_subject));
	}

	const void* LvSubject::getPreviousPointer() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_previous_pointer(const_cast<lv_subject_t*>(&m_subject));
	}

	void LvSubject::initColor(lv_color_t color)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_init_color(&m_subject, color);
		m_initialized = true;
	}

	void LvSubject::setColor(lv_color_t color)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_set_color(&m_subject, color);
	}

	lv_color_t LvSubject::getColor() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_color(const_cast<lv_subject_t*>(&m_subject));
	}

	lv_color_t LvSubject::getPreviousColor() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_previous_color(const_cast<lv_subject_t*>(&m_subject));
	}

	void LvSubject::initGroup(std::span<lv_subject_t*> list)
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_init_group(&m_subject, list.data(), static_cast<uint32_t>(list.size()));
		m_initialized = true;
	}

	lv_subject_t* LvSubject::getGroupElement(int32_t index) const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_get_group_element(const_cast<lv_subject_t*>(&m_subject), index);
	}

	lv_observer_t* LvSubject::addObserver(lv_observer_cb_t observer_cb, void* user_data)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_add_observer(&m_subject, observer_cb, user_data);
	}

	lv_observer_t* LvSubject::addObserverObj(lv_observer_cb_t observer_cb, LvObj& obj, void* user_data)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_add_observer_obj(&m_subject, observer_cb, obj.getRootPtr(), user_data);
	}

	lv_observer_t* LvSubject::addObserverWithTarget(lv_observer_cb_t observer_cb, void* target, void* user_data)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_subject_add_observer_with_target(&m_subject, observer_cb, target, user_data);
	}

	void LvSubject::removeObserver(lv_observer_t* observer)
	{
		ZoneScoped;
		UI_LOCK();
		lv_observer_remove(observer);
	}

	void LvSubject::deinit()
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_deinit(&m_subject);
		m_initialized = false;
	}

	void LvSubject::notify()
	{
		ZoneScoped;
		UI_LOCK();
		lv_subject_notify(&m_subject);
	}

#endif
} // namespace UI
