#include "Model.h"
#include "Comm/JsonDecoder.h"
#include "Comm/Usb.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Hardware/Usb.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/PrinterStatus.h"
#include "Presenter.h"
#include "View.h"
#include "i18n/i18n.h"
#include "lvgl/src/osal/lv_os.h"
#include "nameof.hpp"
#include "tracy/Tracy.hpp"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <new>
#include <queue>
#include <unordered_map>
#include <utility>

// Ensure ResponseType and OM types are complete for payload sizing in this TU
#include "ObjectModel/Alert.h"
#include "Subscribers/ResponseSubscribers.h"
#include "utils/UpgradeHelper.h"

// Define common payload storage privately in this TU to avoid
// instantiating payload tuples with incomplete types in headers.
namespace ModelPayload
{
	template <EventType E>
	struct PayloadInfo
	{
		using tuple_t = typename EventTraits<E>::tuple_type;
		static constexpr size_t size = sizeof(tuple_t);
		static constexpr size_t align = alignof(tuple_t);
	};

	consteval size_t computeMaxSize()
	{
		size_t maxSz = 0;
#define XX(name, ...) maxSz = std::max(maxSz, PayloadInfo<EventType::name>::size);
		EVENTS(XX)
#undef XX
		return std::max(maxSz, PayloadInfo<EventType::Null>::size);
	}

	consteval size_t computeMaxAlign()
	{
		size_t maxAl = 1;
#define XX(name, ...) maxAl = std::max(maxAl, PayloadInfo<EventType::name>::align);
		EVENTS(XX)
#undef XX
		return std::max(maxAl, PayloadInfo<EventType::Null>::align);
	}

	inline constexpr size_t maxPayloadSize = computeMaxSize();
	inline constexpr size_t maxPayloadAlign = computeMaxAlign();

	struct alignas(maxPayloadAlign) PayloadStorage
	{
		std::byte data[maxPayloadSize];
	};
} // namespace ModelPayload

using ModelPayload::PayloadStorage;

struct Model::EventSystem
{
	struct HandlerEntry
	{
		EventHandlerFn fn;
		struct Deleter
		{
			void (*fn)(void*);
			void operator()(void* p) const
			{
				if (fn)
					fn(p);
			}
		};
		std::unique_ptr<void, Deleter> ctx;
	};

	struct Node
	{
		EventType type;
		PayloadStorage payload;
		void (*destroy)(void*);
		void (*move)(void* dst, void* src);

		Node() noexcept
			: type(EventType::Null)
			, destroy(nullptr)
			, move(nullptr)
		{
			ZoneScopedN("Node default constructor");
		}

		~Node()
		{
			ZoneScopedN("Node destructor");
			if (destroy)
			{
				ZoneScopedN("Destroy Event Payload");
				destroy(static_cast<void*>(payload.data));
			}
		}

		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;
		Node& operator=(Node&& other) noexcept
		{
			ZoneScopedN("Node move assignment");
			if (this != &other)
			{
				if (destroy)
				{
					destroy(static_cast<void*>(payload.data));
				}
				type = other.type;
				destroy = other.destroy;
				move = other.move;
				if (other.move)
				{
					ZoneScopedN("Move Event Payload");
					other.move(static_cast<void*>(payload.data), static_cast<void*>(other.payload.data));
					if (other.destroy)
					{
						ZoneScopedN("Destroy Event Payload");
						other.destroy(static_cast<void*>(other.payload.data));
					}
				}
				other.type = EventType::Null;
				other.destroy = nullptr;
				other.move = nullptr;
			}
			return *this;
		}
		Node(Node&& other) noexcept
			: type(other.type)
			, destroy(other.destroy)
			, move(other.move)
		{
			ZoneScopedN("Node move constructor");
			if (other.move)
			{
				ZoneScopedN("Move Event Payload");
				other.move(static_cast<void*>(payload.data), static_cast<void*>(other.payload.data));
				if (other.destroy)
				{
					ZoneScopedN("Destroy Event Payload");
					other.destroy(static_cast<void*>(other.payload.data));
				}
			}
			other.type = EventType::Null;
			other.destroy = nullptr;
			other.move = nullptr;
		}
	};

	std::unordered_map<EventType, std::vector<HandlerEntry>> handlers;
	std::queue<Node> queue;
};

std::atomic<bool> Model::s_instanceInitialized = false;

Model::Model()
{
	ZoneScoped;
	LOG_INFO("Initializing Model...");
	m_events = std::make_unique<EventSystem>();
	// Timers
	if (lv_is_initialized())
	{
		m_timers.tick = lv_timer_create(
			[](lv_timer_t* timer) { static_cast<Model*>(lv_timer_get_user_data(timer))->post<EventType::Tick>(); },
			MODEL_TICK_INTERVAL.count(),
			this);
	}
	else
	{
		LOG_WARN("LVGL not initialized, skipping model tick timer creation");
		m_timers.tick = nullptr;
	}

#if !MULTITHREADED
	m_timers.request =
		lv_timer_create([](lv_timer_t* timer) { static_cast<Model*>(lv_timer_get_user_data(timer))->requestNewData(); },
						Comm::DUET.GetScaledPollInterval(),
						this);
	m_timers.receive = lv_timer_create(
		[](lv_timer_t* timer) { static_cast<Model*>(lv_timer_get_user_data(timer))->receiveNewUsbData(); }, 5, this);
#endif

	// USB drives coming and going, as an event: presenters come and go, so they can't each hold a callback
	USB::UsbMonitor::getInstance().registerCallback([this](const std::string& path, bool mounted)
													{ post<EventType::UsbDrive>(path, mounted); });

	addEventListener<EventType::Connected>(this, &Model::connected);
	addEventListener<EventType::Disconnected>(this, &Model::disconnected);
}

void Model::bind(std::weak_ptr<UI::BasePresenter> presenter)
{
	ZoneScoped;
	UI_LOCK();
	if (presenter.expired())
	{
		LOG_WARN("Attempted to bind an expired presenter");
		return;
	}
	// Prevent duplicate weak_ptr entries for the same presenter
	auto curSp = presenter.lock();
	bool alreadyBound = std::any_of(m_presenters.begin(),
									m_presenters.end(),
									[curSp](const std::weak_ptr<UI::BasePresenter>& wp)
									{
										auto sp = wp.lock();
										return sp && sp == curSp;
									});
	if (alreadyBound)
	{
		LOG_DBG("Presenter {:s} already bound", presenter.lock()->getName());
		return;
	}

	LOG_DBG("Binding presenter '{:s}'", presenter.lock()->getName());
	m_presenters.push_back(presenter);

	// Index this presenter by the events it subscribes to for faster dispatch
	if (auto sp = presenter.lock())
	{
		std::vector<EventType> subscribedEvents;
		const auto& handlers = sp->getEventHandlers();
		subscribedEvents.reserve(handlers.size());
		for (const auto& [et, _] : handlers)
		{
			m_eventPresenterIndex[et].push_back(sp);
			subscribedEvents.push_back(et);
		}
		m_presenterEventIndex[sp.get()] = std::move(subscribedEvents);
	}
}

void Model::unbind(std::weak_ptr<UI::BasePresenter> presenter)
{
	ZoneScoped;
	UI_LOCK();
	if (presenter.expired())
	{
		LOG_WARN("Attempted to unbind an expired presenter");
		return;
	}
	std::shared_ptr<UI::BasePresenter> sharedPresenter = presenter.lock();

	// Remove from optimized event -> presenter index
	{
		auto itIndex = m_presenterEventIndex.find(sharedPresenter.get());
		if (itIndex != m_presenterEventIndex.end())
		{
			for (auto et : itIndex->second)
			{
				auto& vec = m_eventPresenterIndex[et];
				vec.erase(std::remove_if(vec.begin(),
										 vec.end(),
										 [&sharedPresenter](const std::weak_ptr<UI::BasePresenter>& wp)
										 {
											 auto sp = wp.lock();
											 return !sp || sp == sharedPresenter;
										 }),
						  vec.end());
			}
			m_presenterEventIndex.erase(itIndex);
		}
	}

	std::erase_if(m_presenters,
				  [&sharedPresenter](const std::weak_ptr<UI::BasePresenter>& p)
				  {
					  if (p.expired())
					  {
						  LOG_DBG("Unbinding expired presenter");
						  return true;
					  }
					  auto locked = p.lock();
					  if (locked && locked == sharedPresenter)
					  {
						  LOG_DBG("Unbinding presenter '{:s}'", sharedPresenter->getName());
						  return true;
					  }
					  return false;
				  });
}

void Model::startEventLoop()
{
	ZoneScoped;
	if (m_running)
	{
		LOG_WARN("Event loop already running");
		return;
	}
	m_running = true;
	m_eventThread = std::thread(&Model::runEventLoop, this);
}

void Model::stopEventLoop()
{
	ZoneScoped;
	if (!m_running)
	{
		LOG_WARN("Event loop not running");
		return;
	}
	{
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		m_running = false;
	}
	m_eventCondition.notify_all();
	if (m_eventThread.joinable())
	{
		m_eventThread.join();
	}
}

void Model::runEventLoop()
{
	tracy::SetThreadName("Model Event Loop");
	DeadlockDetector::getInstance().allowThreadToTakeMultipleLocks(Log::GetThreadId(), true);

	while (true)
	{
		{
			EventSystem::Node event;
			{
				std::unique_lock<LockableBase(std::mutex)> lock(m_mutex);
				m_eventCondition.wait(lock, [this] { return !m_events->queue.empty() || !m_running; });
				if (!m_running && m_events->queue.empty())
				{
					LOG_DBG("Stopping event loop");
					break;
				}
				event = std::move(m_events->queue.front());
				m_events->queue.pop();
			}

			bool found = false;

			ZoneScoped;
			UI_LOCK();
			{
				ZoneScopedN("Model Event Handlers");
				auto it = m_events->handlers.find(event.type);
				if (it != m_events->handlers.end())
				{
					found = true;
					for (auto& h : it->second)
					{
						ZoneScoped;
						[[maybe_unused]] const auto eventName = nameof::nameof_enum(event.type);
						ZoneName(event.type == EventType::Null ? "Null Event" : eventName.data(), eventName.size());
						ZoneColor(tracy::Color::Yellow);
						h.fn(h.ctx.get(), static_cast<void*>(event.payload.data));
					}
				}
			}

			{
				ZoneScopedN("Presenter Event Handlers");
				auto pit = m_eventPresenterIndex.find(event.type);
				if (pit != m_eventPresenterIndex.end())
				{
					auto& vec = pit->second;
					// Index-based iteration to avoid iterator invalidation pitfalls during erase
					for (size_t i = 0; i < vec.size();)
					{
						auto presenter = vec[i].lock();
						if (!presenter)
						{
							vec.erase(vec.begin() + static_cast<std::ptrdiff_t>(i));
							continue;
						}
						ZoneScoped;
						ZoneName(presenter->getName().data(), presenter->getName().size());
						auto handler = presenter->getEventHandler(event.type);
						if (handler)
						{
							found = true;
							LOG_DBG("Notifying presenter '{:s}' for event '{:s}'",
									presenter->getName(),
									nameof::nameof_enum(event.type));

							ZoneScoped;
							[[maybe_unused]] const auto eventName = nameof::nameof_enum(event.type);
							ZoneName(event.type == EventType::Null ? "Null Event" : eventName.data(), eventName.size());
							ZoneColor(tracy::Color::Red);
							handler.fn(handler.user, static_cast<void*>(event.payload.data));
						}
						++i;
					}
				}
			}

			if (!found)
			{
				LOG_VERBOSE("No handler for event type {:s}", nameof::nameof_enum(event.type));
			}
		}
	}
}

std::chrono::milliseconds Model::requestNewData()
{
	ZoneScoped;
	bool seqAvailable = Comm::sendNext();
#if 0
	if (seqAvailable && Comm::DUET.GetCommunicationType() == Comm::CommunicationType::network)
	{
		return 50; // 50ms
	}
#else
	UNUSED(seqAvailable);
#endif
	return Comm::DUET.GetScaledPollInterval();
}

void Model::runSubscribers(const char* key, Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	auto& subscribers = getSubscribers(key);
	if (subscribers.size() != 0)
	{
		LOG_DBG("found {:d} subscribers for '{:s}'", subscribers.size(), key);
		for (auto& subscriber : subscribers)
		{
			ZoneScopedN("Running subscriber");
			subscriber.run(decoder, data, indices);
		}
	}
}

void Model::runArrayEndSubscribers(const char* key, Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	auto& subscribers = getArrayEndSubscribers(key);
	if (subscribers.size() != 0)
	{
		LOG_DBG("found {:d} array end subscribers for '{:s}'", subscribers.size(), key);
		for (auto& subscriber : subscribers)
		{
			ZoneScopedN("Running subscriber");
			subscriber.run(decoder, indices);
		}
	}
}

void Model::connected()
{
	ZoneScoped;
	LOG_DBG("Connected event");
	if (StorageHelper::getData(ID_DISPLAY_CONNECTED_MESSAGE))
	{
		post<EventType::Response>(ResponseType::INFO, _("message.connected"));
	}
}

void Model::registerHandler(EventType e, EventHandlerFn fn, void* user, void (*deleter)(void*)) noexcept
{
	std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
	EventSystem::HandlerEntry::Deleter d{deleter};
	m_events->handlers[e].emplace_back(
		EventSystem::HandlerEntry{fn, std::unique_ptr<void, EventSystem::HandlerEntry::Deleter>(user, d)});
}

void Model::enqueueEvent(EventType e, void* payload) noexcept
{
	ZoneScoped;
	std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
	EventSystem::Node node;
	node.type = e;
	// Copy-construct the typed payload into storage and remember how to destroy/move it
	switch (e)
	{
#define XX(name, ...)                                                                                                  \
	case EventType::name:                                                                                              \
	{                                                                                                                  \
		ZoneScopedN("Constructing Event Payload");                                                                     \
		using Tuple = typename EventTraits<EventType::name>::tuple_type;                                               \
		::new (static_cast<void*>(node.payload.data)) Tuple(std::move(*static_cast<Tuple*>(payload)));                 \
		node.destroy = [](void* p) { static_cast<Tuple*>(p)->~Tuple(); };                                              \
		node.move = [](void* dst, void* src) { new (dst) Tuple(std::move(*static_cast<Tuple*>(src))); };               \
		break;                                                                                                         \
	}
		EVENTS(XX)
#undef XX
	default:
	{
		using Tuple = typename EventTraits<EventType::Null>::tuple_type;
		::new (static_cast<void*>(node.payload.data)) Tuple();
		node.destroy = [](void* p) { static_cast<Tuple*>(p)->~Tuple(); };
		node.move = [](void* dst, void* src) { new (dst) Tuple(std::move(*static_cast<Tuple*>(src))); };
		break;
	}
	}
	m_events->queue.emplace(std::move(node));
	m_eventCondition.notify_one();
}

void Model::disconnected()
{
	ZoneScoped;
	LOG_DBG("Disconnected event");
	if (StorageHelper::getData(ID_DISPLAY_CONNECTED_MESSAGE))
	{
		post<EventType::Response>(ResponseType::INFO, _("message.disconnected"));
	}
}

Model::~Model()
{
	ZoneScoped;
	// Ensure the event loop thread is stopped cleanly before destruction
	stopEventLoop();

	std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
	// Flush any remaining queued events to properly destroy payloads
	while (!m_events->queue.empty())
	{
		m_events->queue.pop();
	}
	// unique_ptr takes care of handler context cleanup
}
