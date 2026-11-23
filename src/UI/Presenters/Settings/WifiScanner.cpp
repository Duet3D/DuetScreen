/*
 * WifiScanner.cpp
 */

#include "WifiScanner.h"
#include "Debug.h"
#include "UI/Core/Model.h"

namespace UI
{
	/** How often networks are scanned for while scanning */
	static constexpr std::chrono::seconds SCAN_INTERVAL{3};

	WifiScanner::WifiScanner()
		: m_scanThread([this](std::stop_token stop) { scanLoop(stop); })
	{
	}

	WifiScanner::~WifiScanner()
	{
		m_scanThread.request_stop();
		m_wake.notify_all();
		if (m_connectThread.joinable())
		{
			m_connectThread.request_stop();
			m_connectThread.join();
		}
		if (m_scanThread.joinable())
		{
			m_scanThread.join();
		}
	}

	void WifiScanner::setScanning(bool scanning)
	{
		{
			std::lock_guard lock(m_mutex);
			m_scanning = scanning;
			m_scanRequested = m_scanRequested || scanning;
		}
		m_wake.notify_all();
	}

	void WifiScanner::scanNow()
	{
		{
			std::lock_guard lock(m_mutex);
			m_scanRequested = true;
		}
		m_wake.notify_all();
	}

	std::vector<WiFiNetwork> WifiScanner::networks() const
	{
		std::lock_guard lock(m_mutex);
		return m_networks;
	}

	void WifiScanner::connect(std::string ssid, std::optional<std::string> password)
	{
		if (m_connecting.exchange(true))
		{
			return; // One attempt at a time
		}
		if (m_connectThread.joinable())
		{
			m_connectThread.join(); // The last attempt has finished
		}
		m_connectThread = std::jthread(
			[this, ssid = std::move(ssid), password = std::move(password)](std::stop_token stop)
			{
				tracy::SetThreadName("WifiConnectThread");
				const auto result = password ? NetworkHelper::connect(ssid, *password) : NetworkHelper::connect(ssid);
				m_connecting.store(false);
				if (stop.stop_requested())
				{
					return;
				}
				Model::get().post<EventType::WifiConnectResult>(ssid, result);
				scanNow();
			});
	}

	void WifiScanner::scanLoop(std::stop_token stop)
	{
		tracy::SetThreadName("WifiScanThread");
		std::unique_lock lock(m_mutex);
		while (!stop.stop_requested())
		{
			// Until a scan is asked for, or it is time for the next
			if (m_scanning)
			{
				m_wake.wait_for(lock, stop, SCAN_INTERVAL, [this] { return m_scanRequested; });
			}
			else
			{
				m_wake.wait(lock, stop, [this] { return m_scanRequested || m_scanning; });
			}
			if (stop.stop_requested())
			{
				break;
			}
			if (!m_scanning && !m_scanRequested)
			{
				continue;
			}
			m_scanRequested = false;

			lock.unlock();
			std::vector<WiFiNetwork> found = NetworkHelper::scanWiFiNetworks();
			lock.lock();
			m_networks = std::move(found);
			Model::get().post<EventType::WifiNetworks>();
		}
	}
} // namespace UI
