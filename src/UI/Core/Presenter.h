#pragma once

#include "ModelListener.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace UI
{
#define PRESENTER_CONSTRUCTOR(PresenterType, ViewType)                                                                 \
	PresenterType(LvObj* view)                                                                                         \
		: Presenter<ViewType>(view)                                                                                    \
	{                                                                                                                  \
	}

	class LvObj;

	class BasePresenter : public ModelListener
	{
	  public:
		enum class State
		{
			UNINITIALISED = 0,
			DEACTIVE,
			DEACTIVATING,
			ACTIVATING,
			ACTIVE
		};

		void init();
		void activate();
		void deactivate();

		virtual std::string_view getName() const;

		State getState() const { return m_state; }
		bool isActive() const { return m_state == State::ACTIVE; }

		void connected();
		void disconnected();

	  protected:
		virtual void onInit() {}
		virtual void onActivate() {}
		virtual void onDeactivate() {}

		virtual void onConnect() {}
		virtual void onDisconnect() {}

		volatile State m_state = State::UNINITIALISED;
	};

	template <class V>
	class Presenter : public BasePresenter
	{
	  public:
		Presenter(LvObj* view)
			: m_view(static_cast<V*>(view))
		{
		}

		void activate()
		{
			if (m_view != nullptr)
			{
				BasePresenter::activate();
				Model::get().bind(m_view->getPresenter());
			}
		}

		void deactivate()
		{
			if (m_view != nullptr)
			{
				BasePresenter::deactivate();
				Model::get().unbind(m_view->getPresenter());
			}
		}

		std::string_view getName() const final
		{
			if (m_view == nullptr)
			{
				static std::string empty = "";
				return empty;
			}

			return m_view->getName();
		}

		void setView(V* view) { m_view = view; }

		V* getView() const { return m_view; }

	  protected:
		V* m_view;
	};
} // namespace UI
