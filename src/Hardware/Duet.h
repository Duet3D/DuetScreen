/*
 * Duet.h
 *
 *  Created on: 26 Jan 2024
 *      Author: Andy Everitt
 */

#ifndef JNI_HARDWARE_DUET_H_
#define JNI_HARDWARE_DUET_H_

#include "Configuration.h"
#include "Duet3D/General/String.h"
#include "Duet3D/General/StringRef.h"
#include "termios.h"
#include "tracy/Tracy.hpp"
#include "utils/utils.h"
#include <fmt/format.h>
#include <hv/requests.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <thread>

namespace Comm
{
	constexpr std::string_view const duetCommunicationTypeNames[] = {
		"connection_method.usb", "connection_method.network", "connection_method.uart"};

	typedef struct
	{
		int rate;
		speed_t internal;
	} baudrate_t;

	constexpr baudrate_t baudRates[] = {{1200, B1200},
										{2400, B2400},
										{4800, B4800},
										{9600, B9600},
										{19200, B19200},
										{38400, B38400},
										{57600, B57600},
										{115200, B115200},
										{230400, B230400},
#ifndef __APPLE__
										{460800, B460800},
										{921600, B921600}
#endif
	};

	enum class CommunicationType
	{
		usb,
		network,
		uart,
	};

	static_assert(magic_enum::enum_count<CommunicationType>() == std::size(duetCommunicationTypeNames),
				  "duetCommunicationTypeNames must have the same number of elements as CommunicationType");

	struct DuetConfig
	{
		std::string ipAddress = std::string(DEFAULT_IP_ADDRESS);
		std::string password = "";
		std::chrono::milliseconds pollInterval = DEFAULT_PRINTER_POLL_INTERVAL;
		CommunicationType communicationType = CommunicationType::usb;
		speed_t baudRate = B115200;
	};

	class Duet
	{
	  public:
		typedef int32_t error_code;

		static Duet& GetInstance()
		{
			static Duet instance;
			return instance;
		}

		void Init();
		void Reset();
		void Reconnect();

		void SetCommunicationType(CommunicationType type);
		CommunicationType GetCommunicationType() const;
		std::string_view GetCommunicationTypeName() const;
		void SetPollInterval(std::chrono::milliseconds interval);
		void ScalePollIntervalScale(float scale);
		std::chrono::milliseconds GetPollInterval() const;
		std::chrono::milliseconds GetScaledPollInterval() const;
		uint32_t GetNextLineNumber() { return m_nextLineNumber++; }

		void Estop();

		void SendGcode(std::string_view gcode, bool force = false);

		template <typename... Args>
		void SendGcodef(fmt::format_string<Args...> fmt, Args&&... args)
		{
			std::string formatted = fmt::format(fmt, std::forward<Args>(args)...);
			SendGcode(formatted);
		}

		void RequestReply(HttpResponse& r);
		void ProcessReply(HttpResponse& r);

		bool UploadFile(std::string_view filename, const std::string& contents);
		bool DownloadFile(std::string_view filename, std::function<void(const std::string&)> onComplete);
		bool DeleteFile(std::string_view filename);

		void RequestModel(std::string_view flags = "d99f");
		void RequestModel(std::string_view key, std::string_view flags);
		bool RequestFileList(std::string_view dir, const size_t first = 0);
		bool RequestFileInfo(std::string_view filename);
		bool RequestThumbnail(std::string_view filename, uint32_t offset);

		// UART methods
		void SetBaudRate(const speed_t baudRateCode);
		void SetBaudRate(const baudrate_t& baudRate);
		const baudrate_t& GetBaudRate() const;

		// Network methods
		bool Connect(bool useSessionKey = true);
		bool Disconnect();
		bool IsDisconnected() const { return m_connectionState == ConnectionState::DISCONNECTED; }
		bool IsConnected() const { return m_connectionState == ConnectionState::CONNECTED; }
		bool IsConnecting() const { return m_connectionState == ConnectionState::CONNECTING; }
		bool IsSbcMode() const { return m_sbcMode; }

		const std::string& GetBaseUrl() const;

		void SetIPAddress(std::string_view ipAddress);
		const std::string& GetIPAddress() const;
		void ClearIPAddress();

		void SetPassword(std::string_view password);
		const std::string& GetPassword() const;

		void SetSessionKey(const uint32_t sessionKey);

		// USB methods

	  private:
		Duet();

		void PrepareRequest(HttpRequest& req, std::string_view subUrl, hv::QueryParams& queryParameters);
		bool AsyncGet(std::string_view subUrl, hv::QueryParams& queryParameters, HttpResponseCallback callback);
		void AsyncGetInner(const HttpRequestPtr& req, HttpResponseCallback callback);
		bool AsyncGetCallback(const HttpRequestPtr& req, const HttpResponsePtr& r, HttpResponseCallback callback);
		bool Get(std::string_view subUrl, HttpResponse& r, hv::QueryParams& queryParameters);
		bool Post(std::string_view subUrl, HttpResponse& r, hv::QueryParams& queryParameters, std::string_view data);

		DuetConfig m_config;
		std::chrono::milliseconds m_lastRequestTime;
		float m_pollIntervalScale;
		uint32_t m_nextLineNumber = 0;
		TracyLockable(std::mutex, m_sendLock);

		// USB

		// Network
		hv::HttpClient m_cli; // for sendAsync() only!
		uint32_t m_sessionKey;
		std::chrono::milliseconds m_sessionTimeout;
		bool m_sbcMode;
		enum class ConnectionState
		{
			DISCONNECTED,
			CONNECTING,
			CONNECTED
		} m_connectionState = ConnectionState::DISCONNECTED;

		static constexpr uint32_t sm_noSessionKey = 0;
	};

#define DUET Duet::GetInstance()
} // namespace Comm

#endif /* JNI_HARDWARE_DUET_H_ */
