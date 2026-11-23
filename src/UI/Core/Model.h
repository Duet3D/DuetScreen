#pragma once

#include "Comm/Responses.h"
#include "Debug.h"
#include "LockWrapper.h"
#include "UI/Core/EventTypes.h"
#include "lvgl/lvgl.h"
#include "nameof.hpp"
#include "tracy/Tracy.hpp"
#include <atomic>
#include <condition_variable>
#include <fmt/ostream.h>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <tuple>
#include <unordered_map>

namespace UI
{
	class BasePresenter;
}

/*
To add an Event, edit UI/Core/EventTypes.h (EVENTS macro).

An event can be triggered with Model::get().post<EventType::EventName>(args...).
Listeners can be registered with addEventListener<EventType::EventName>(...).

Payloads are copied into an internal queue with fixed-size storage; dispatch is
via a direct function pointer call for zero overhead at runtime.
*/

// Thin, non-templated handler signature (user data + opaque payload pointer)
using EventHandlerFn = void (*)(void* user, const void* payload);

struct EventHandler
{
	EventHandlerFn fn = nullptr;
	void* user = nullptr;
	explicit operator bool() const noexcept { return fn != nullptr; }
};

// Note: Common payload storage is defined privately in Model.cpp.

class Model
{
  public:
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;
	~Model();

	static Model& get()
	{
		ZoneScoped;
		static Model instance;
		s_instanceInitialized = true;
		return instance;
	}

	static bool isInitialized() { return s_instanceInitialized; }

	/**
	 * @brief Add a `Presenter` to listen to events
	 * @param presenter
	 */
	void bind(std::weak_ptr<UI::BasePresenter> presenter);

	/**
	 * @brief Remove a `Presenter`
	 * @param presenter
	 */
	void unbind(std::weak_ptr<UI::BasePresenter> presenter);

	void startEventLoop();
	void stopEventLoop();

	template <EventType E, typename Func>
		requires UI::InvocableFromTuple<std::decay_t<Func>, typename EventTraits<E>::tuple_type>
	void addEventListener(Func&& func)
	{
		ZoneScoped;
		using Tuple = typename EventTraits<E>::tuple_type;

		using F = std::decay_t<Func>;
		auto fobj = std::make_unique<F>(std::forward<Func>(func));
		auto thunk = [](void* user, const void* payload)
		{
			const auto& tup = *static_cast<const Tuple*>(payload);
			std::apply(*static_cast<F*>(user), tup);
		};
		registerHandler(E, thunk, fobj.release(), [](void* p) { delete static_cast<F*>(p); });
	}

	template <EventType E, typename Class, typename... Args>
		requires UI::ExactArgsMatch<typename EventTraits<E>::tuple_type, Args...>
	void addEventListener(Class* instance, void (Class::*memberFunc)(Args...))
	{
		ZoneScoped;
		using ExpectedTuple = typename EventTraits<E>::tuple_type;

		struct Ctx
		{
			Class* instance;
			void (Class::*mf)(Args...);
		};
		auto ctx = std::make_unique<Ctx>(Ctx{instance, memberFunc});
		auto thunk = [](void* user, const void* payload)
		{
			const auto& tup = *static_cast<const ExpectedTuple*>(payload);
			auto* c = static_cast<Ctx*>(user);
			std::apply([c](const auto&... a) { (c->instance->*c->mf)(a...); }, tup);
		};
		registerHandler(E, thunk, ctx.release(), [](void* p) { delete static_cast<Ctx*>(p); });
	}

	template <EventType E, typename... Args>
		requires UI::ExactArgsMatch<typename EventTraits<E>::tuple_type, Args...>
	void post(Args&&... args)
	{
		[[maybe_unused]] constexpr auto eventName = nameof::nameof_enum<E>();
		ZoneScopedNC(eventName.data(), tracy::Color::Red);

		using Tuple = typename EventTraits<E>::tuple_type;
		Tuple payload{std::forward<Args>(args)...};
		enqueueEvent(E, &payload);
	}

	void runEventLoop();

  private:
	Model();

	void tick();

	/* presenter callbacks */

	void connected();
	void disconnected();

	std::list<std::weak_ptr<UI::BasePresenter>> m_presenters;

	// Optimized event dispatch: map event -> presenters subscribed to that event
	std::unordered_map<EventType, std::vector<std::weak_ptr<UI::BasePresenter>>> m_eventPresenterIndex;
	// Reverse index to support fast unbind cleanup
	std::unordered_map<UI::BasePresenter*, std::vector<EventType>> m_presenterEventIndex;

	// Internal event system (hidden implementation to reduce compile-time impact)
	struct EventSystem;
	std::unique_ptr<EventSystem> m_events;
	std::condition_variable_any m_eventCondition;
	std::thread m_eventThread;
	std::atomic<bool> m_running{false};
	TracyLockable(std::mutex, m_mutex);

	struct
	{
		lv_timer_t* tick;
	} m_timers;

	static std::atomic<bool> s_instanceInitialized;

	// Non-templated registration/enqueue used by thin template wrappers
	void registerHandler(EventType e, EventHandlerFn fn, void* user, void (*deleter)(void*)) noexcept;
	void enqueueEvent(EventType e, void* payload) noexcept;
};

#define MODEL_LOCK()                                                                                                   \
	LOG_VERBOSE("MODEL_LOCK requested in thread {}", Log::GetThreadId());                                              \
	std::lock_guard<LockableBase(DeadlockDetectingMutex<std::recursive_mutex>)> modelLock(mutexModel);
