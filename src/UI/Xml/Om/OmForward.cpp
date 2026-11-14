/*
 * OmForward.cpp
 */

#include "OmForward.h"
#include "OmBindings.h"
#include "OmList.h"

namespace UI::Xml
{
	OmForward::OmForward(OmBindings& bindings, OmBoundPath path, const Converter& converter, std::string argument)
		: m_bindings(bindings)
		, m_path(std::move(path))
		, m_converter(converter)
		, m_argument(std::move(argument))
	{
		if (m_converter.type == Converter::Type::integer)
		{
			m_integer = std::make_unique<IntSubject>();
			m_own = m_integer.get();
		}
		else
		{
			m_textSubject = std::make_unique<StringSubject<OmField::TEXT_SIZE>>();
			m_own = m_textSubject.get();
		}
		showMissing();
		// Adding an observer calls it, which follows the field: m_path is complete by then
		for (lv_subject_t* subject : m_path.subjects())
		{
			m_indexObservers.push_back(lv_subject_add_observer(subject, indexChangedCb, this));
		}
	}

	OmForward::~OmForward()
	{
		for (lv_observer_t* observer : m_indexObservers)
		{
			lv_observer_remove(observer);
		}
		if (m_targetObserver)
		{
			lv_observer_remove(m_targetObserver);
		}
	}

	void OmForward::indexChangedCb(lv_observer_t* observer, lv_subject_t* /* subject */)
	{
		static_cast<OmForward*>(lv_observer_get_user_data(observer))->retarget();
	}

	void OmForward::targetChangedCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* forward = static_cast<OmForward*>(lv_observer_get_user_data(observer));
		if (forward->m_integer)
		{
			forward->m_integer->set(lv_subject_get_int(subject));
		}
		else
		{
			forward->m_textSubject->set(lv_subject_get_string(subject));
		}
	}

	void OmForward::retarget()
	{
		const auto target = m_path.resolve();
		if (target == m_target && (m_targetObserver != nullptr || !target))
		{
			return;
		}
		if (m_targetObserver)
		{
			lv_observer_remove(m_targetObserver);
			m_targetObserver = nullptr;
		}
		m_target = target;

		Subject* field = target ? m_bindings.subject(*target, m_converter.name, m_argument) : nullptr;
		if (field == nullptr)
		{
			showMissing(); // Points at nothing, e.g. no current tool
			return;
		}
		// Adding the observer calls it, which copies the field
		m_targetObserver = lv_subject_add_observer(field->raw(), targetChangedCb, this);
	}

	void OmForward::showMissing()
	{
		if (m_integer)
		{
			m_integer->set(m_converter.toInteger(OM::Node::missing(), m_target.value_or(OM::Path()), m_argument));
		}
		else
		{
			m_textSubject->set(m_converter.toText(OM::Node::missing(), m_target.value_or(OM::Path()), m_argument));
		}
	}

	OmForwardList::OmForwardList(OmBindings& bindings, OmBoundPath path)
		: m_bindings(bindings)
		, m_path(std::move(path))
	{
		for (lv_subject_t* subject : m_path.subjects())
		{
			m_indexObservers.push_back(lv_subject_add_observer(subject, indexChangedCb, this));
		}
	}

	OmForwardList::~OmForwardList()
	{
		for (lv_observer_t* observer : m_indexObservers)
		{
			lv_observer_remove(observer);
		}
		if (m_target)
		{
			m_target->removeListener(this);
		}
	}

	size_t OmForwardList::size() const
	{
		return m_target ? m_target->size() : 0;
	}

	Row& OmForwardList::row(size_t index)
	{
		return m_target->row(index);
	}

	void OmForwardList::rowFields(FieldVisitor& f) const
	{
		OmRow example;
		example.allFields(f);
	}

	void OmForwardList::indexChangedCb(lv_observer_t* observer, lv_subject_t* /* subject */)
	{
		static_cast<OmForwardList*>(lv_observer_get_user_data(observer))->retarget();
	}

	void OmForwardList::retarget()
	{
		const auto targetPath = m_path.resolve();
		if (targetPath == m_targetPath && (m_target != nullptr || !targetPath))
		{
			return;
		}
		// The old array's rows leave first (while they still exist), then the new array's arrive
		if (m_target)
		{
			if (const size_t old = m_target->size(); old > 0)
			{
				notifyResized(old, 0);
			}
			m_target->removeListener(this);
		}
		m_targetPath = targetPath;
		m_target = targetPath ? m_bindings.list(*targetPath) : nullptr;
		if (m_target)
		{
			m_target->addListener(this);
			if (const size_t size = m_target->size(); size > 0)
			{
				notifyResized(0, size);
			}
		}
	}

	void OmForwardList::onListResized(size_t oldSize, size_t newSize)
	{
		notifyResized(oldSize, newSize);
	}

	void OmForwardList::onListDestroyed()
	{
		if (const size_t old = m_target ? m_target->size() : 0; old > 0)
		{
			notifyResized(old, 0);
		}
		m_target = nullptr;
		m_targetPath.reset();
	}
} // namespace UI::Xml
