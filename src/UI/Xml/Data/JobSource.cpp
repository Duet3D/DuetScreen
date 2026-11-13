/*
 * JobSource.cpp
 */

#include "JobSource.h"
#include "ObjectModel/Job.h"
#include "UI/Core/Model.h"

namespace UI::Xml
{
	JobSource::JobSource()
		: DataSource("job")
	{
	}

	void JobSource::fields(FieldVisitor& f)
	{
		f.field("job.file", m_file, "The current job's file; empty when there is none");
	}

	void JobSource::onInit()
	{
		registerEventListener<EventType::JobFileName>(this, &JobSource::fileChanged);
	}

	void JobSource::update()
	{
		MODEL_LOCK();
		fileChanged(OM::GetJobName());
	}

	void JobSource::clear()
	{
		m_file.set("");
	}

	void JobSource::fileChanged(const std::string& file)
	{
		m_file.set(file);
	}
} // namespace UI::Xml
