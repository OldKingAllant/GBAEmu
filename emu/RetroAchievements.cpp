#include "RetroAchievements.hpp"
#include "Emulator.hpp"

#include <fmt/format.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <Poco/StreamCopier.h>

#include <filesystem>
#include <fstream>
#include <chrono>

#include "../thirdparty/mini/ini.h"

#include "../rcheevos/include/rc_hash.h"

namespace GBA::emulation {
	namespace detail {
		static uint32_t read_memory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client)
		{
			//fmt::println("[ACHIEVEMENTS] Read at {:#010x}", address);
			/*
			{ 0x000000U, 0x007FFFU, 0x03000000U, RC_MEMORY_TYPE_SYSTEM_RAM, "System RAM" }, 
			{ 0x008000U, 0x047FFFU, 0x02000000U, RC_MEMORY_TYPE_SYSTEM_RAM, "System RAM" }, 
			{ 0x048000U, 0x057FFFU, 0x0E000000U, RC_MEMORY_TYPE_SAVE_RAM, "Save RAM" }     
			*/
			auto between = [](u32 address, u32 first, u32 last) {
				return address >= first && address < last;
			};
			auto region = 
				between(address, 0x000000, 0x008000) ? memory::MEMORY_RANGE::IWRAM : 
				between(address, 0x008000, 0x048000) ? memory::MEMORY_RANGE::EWRAM :
				memory::MEMORY_RANGE::BIOS;
			address -= region == memory::MEMORY_RANGE::EWRAM ? 0x008000 : 0x0;

			if (region == memory::MEMORY_RANGE::BIOS ||
				address + num_bytes > memory::REGIONS_LEN[u8(region)]) {
				fmt::println("[ACHIEVEMENTS] Reading invalid address {:#010x}",
					address);
				return 0;
			}

			auto ra  = std::bit_cast<RetroAchievements*>( rc_client_get_userdata(client) );
			auto emu = ra->GetEmulator();
			auto& bus = emu->GetContext().bus;

			auto original_num_bytes = num_bytes;

			while (num_bytes--) {
				auto curr_address = (u32(region) << 24) + address;
				*buffer = bus.ReadFast<u8, false>(curr_address);
				++buffer; ++address;
			}

			return original_num_bytes;
		}

		static void http_get(Poco::Net::HTTPClientSession& session,
			Poco::Net::HTTPRequest& request, rc_client_server_callback_t callback,
			void* callback_data) {
			Poco::Net::HTTPResponse response{};
			session.setKeepAlive(false);
			session.sendRequest(request);


			auto& response_stream = session.receiveResponse(response);
			std::ostringstream response_body_os{};
			std::string curr_line{};

			while (std::getline(response_stream, curr_line)) {
				response_body_os << curr_line;
			}

			std::string response_body{ response_body_os.str() };

			rc_api_server_response_t server_response{};
			memset(&server_response, 0, sizeof(server_response));
			server_response.body = std::bit_cast<const char*>(response_body.data());
			server_response.body_length = size_t(response_body.length());
			server_response.http_status_code = int(response.getStatus());

			callback(&server_response, callback_data);
		}

		static void http_post(Poco::Net::HTTPClientSession& session,
			Poco::Net::HTTPRequest& request, 
			const char* body, 
			rc_client_server_callback_t callback,
			void* callback_data) {
			std::string body_str{ body };
			request.setContentLength(body_str.length());

			Poco::Net::HTTPResponse response{};
			session.setKeepAlive(false);
			session.sendRequest(request) << body_str;

			
			auto& response_stream = session.receiveResponse(response);
			std::ostringstream response_body_os{};
			std::string curr_line{};
			
			while (std::getline(response_stream, curr_line)) {
				response_body_os << curr_line;
			}

			std::string response_body{ response_body_os.str() };

			rc_api_server_response_t server_response{};
			memset(&server_response, 0, sizeof(server_response));
			server_response.body = std::bit_cast<const char*>(response_body.data());
			server_response.body_length = size_t( response_body.length() );
			server_response.http_status_code = int( response.getStatus() );

			callback(&server_response, callback_data);
		}

		// This is the HTTP request dispatcher that is provided to the rc_client. Whenever the client
		// needs to talk to the server, it will call this function.
		static void server_call(const rc_api_request_t* request,
			rc_client_server_callback_t callback, void* callback_data, rc_client_t* client)
		{
			// RetroAchievements may not allow hardcore unlocks if we don't properly identify ourselves.
			const char* user_agent = "NoBiosEmu/1.0";

			Poco::Net::HTTPClientSession session{};
			Poco::URI uri{request->url};
			session.setHost(uri.getHost());
			session.setPort(80);
			Poco::Net::HTTPRequest poco_req{};

			poco_req.setMethod(request->post_data ? 
				Poco::Net::HTTPRequest::HTTP_POST :
				Poco::Net::HTTPRequest::HTTP_GET);
			poco_req.setURI(uri.getPathAndQuery());
			poco_req.setVersion(Poco::Net::HTTPRequest::HTTP_1_1);

			poco_req.set("User-Agent", user_agent);
			poco_req.setContentType(std::string{ request->content_type });

			// If post data is provided, we need to make a POST request, otherwise, a GET request will suffice.
			if (request->post_data) {
				http_post(session, poco_req, request->post_data, callback, callback_data);
			}
			else {
				http_get(session, poco_req, callback, callback_data);
			}
		}

		// Write log messages to the console
		static void log_message(const char* message, const rc_client_t* client)
		{
			fmt::println("[ACHIEVEMENTS] {}", message);
		}

		static void login_callback(int result, const char* error_message, rc_client_t* client, void* userdata)
		{
			// If not successful, just report the error and bail.
			if (result != RC_OK)
			{
				fmt::println("[ACHIEVEMENTS] Login failed: {}", error_message);
				return;
			}

			// Login was successful. Capture the token for future logins so we don't have to store the password anywhere.
			const rc_client_user_t* user = rc_client_get_user_info(client);
			//store_retroachievements_credentials(user->username, user->token);

			// Inform user of successful login
			fmt::println("[ACHIEVEMENTS] Logged in as {} ({} points)", user->display_name, user->score);

			RetroAchievements* ra = std::bit_cast<RetroAchievements*>(userdata);
			ra->SetLoggedIn();
			ra->FillUserInfo(user);
		}

		static void load_game_callback(int result, const char* error_message, rc_client_t* client, void* userdata)
		{
			if (result != RC_OK)
			{
				fmt::println("[ACHIEVEMENTS] RetroAchievements game load failed: {}", 
					error_message);
				return;
			}

			RetroAchievements* ra = std::bit_cast<RetroAchievements*>(userdata);
			ra->SetGameLoaded();
		}

		static void event_handler(const rc_client_event_t* event, rc_client_t* client)
		{
			auto ra = std::bit_cast<RetroAchievements*>(
				rc_client_get_userdata(client)
			);
			ra->ClientEvent(event);
		}
	}

	RetroAchievements::RetroAchievements(Emulator* emu, std::string const& credentials_file) :
		mastery_achieved_interrupt{false},
		m_emu{emu}, m_rc_client{nullptr}, 
		m_logged_in{false}, m_user{},
		m_rom_hash{}, m_game_loaded{false},
		m_cache_path{"./rc_cache"},
		m_game_info{nullptr},
		m_game_summary{},
		m_achievements{},
		m_new_unlocked_achievements{},
		m_mastery_achieved{false},
		m_achievement_progress_update{}
	{
		if (!std::filesystem::exists(m_cache_path)) {
			std::filesystem::create_directories(m_cache_path);
		}

		m_rc_client = rc_client_create(detail::read_memory, detail::server_call);

		if (m_rc_client == nullptr) {
			throw std::runtime_error("Failed RC client init");
		}

		rc_client_enable_logging(m_rc_client, RC_CLIENT_LOG_LEVEL_VERBOSE, detail::log_message);
		rc_client_set_event_handler(m_rc_client, detail::event_handler);
		rc_client_set_hardcore_enabled(m_rc_client, 0);
		rc_client_set_userdata(m_rc_client, this);

		PerformLogin(credentials_file);
	}

	RetroAchievements::~RetroAchievements() {
		if(m_logged_in) rc_client_logout(m_rc_client);
		rc_client_destroy(m_rc_client);
		m_rc_client = nullptr;
		m_logged_in = false;
		m_user = {};
	}

	void RetroAchievements::FillUserInfo(rc_client_user_t const* client_user) {
		m_user.connect_token = std::string{ client_user->token };
		m_user.user_internal = *client_user;
	}

	void RetroAchievements::LoadGame(u8* rom_base, std::size_t rom_size) {
		std::string rom_path{m_emu->GetContext().pack.GetRomPath().string()};
		char hash[33] = {};
		rc_hash_iterator_t iter{};
		rc_hash_initialize_iterator(&iter, rom_path.c_str(),
			rom_base, rom_size);
		auto hash_success{rc_hash_generate(hash, RC_CONSOLE_GAMEBOY_ADVANCE,
			&iter)};

		if (hash_success == 0) {
			fmt::println("[ACHIEVEMENTS] Game load failed, could not generate hash");
			return;
		}

		m_rom_hash = std::string{hash};

		if (!std::filesystem::exists(GetGameCachePath())) {
			std::filesystem::create_directories(GetGameCachePath());
		}

		rc_client_begin_load_game(m_rc_client, hash,
			detail::load_game_callback, this);
		LoadGameInfo();
		LoadAchievementList(true);
	}

	void RetroAchievements::LoadGameInfo() {
		m_game_info = rc_client_get_game_info(m_rc_client);
		auto badge_path = GetGameBadgePath();
		DownloadImage(m_game_info->badge_url, badge_path);
		rc_client_get_user_game_summary(m_rc_client, &m_game_summary);
		m_mastery_achieved = m_game_summary.num_core_achievements ==
			m_game_summary.num_unlocked_achievements;
	}

	void RetroAchievements::LoadAchievementList(bool download_badges) {
		m_achievements.clear();

		rc_client_achievement_list_t* list = rc_client_create_achievement_list(
			m_rc_client,
			RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE_AND_UNOFFICIAL,
			RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_PROGRESS);

		for (auto bucket = 0U; bucket < list->num_buckets; ++bucket) {
			auto const& curr_bucket = list->buckets[bucket];
			for (auto ach = 0U; ach < curr_bucket.num_achievements; ++ach) {
				auto curr_achievement = curr_bucket.achievements[ach];
				RA_Achievement ach_descriptor{};
				ach_descriptor.bucket_label = std::string{curr_bucket.label};
				ach_descriptor.name         = std::string{curr_achievement->badge_name};

				ach_descriptor.type = curr_achievement->category ==
					RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL ?
					RA_AchievementType::UNOFFICIAL :
					RA_AchievementType::CORE;

				if (curr_achievement->bucket == RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED) {
					ach_descriptor.progress = "Unsupported";
					ach_descriptor.type     = RA_AchievementType::UNSUPPORTED;
				}
				else if (curr_achievement->unlocked)
					ach_descriptor.progress = "Unlocked";
				else if (curr_achievement->measured_percent)
					ach_descriptor.progress = curr_achievement->measured_progress;
				else
					ach_descriptor.progress = "Locked";

				ach_descriptor.unlocked    = bool(curr_achievement->unlocked);
				ach_descriptor.description = std::string{curr_achievement->description};

				if (download_badges) {
					char url[256] = {};
					if (rc_client_achievement_get_image_url(curr_achievement,
						RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE, url, sizeof(url)) == RC_OK) {
						std::string path = GetGameCachePath();
						path += fmt::format("/{}_locked.png",
							ach_descriptor.name);

						DownloadImage(url, path);
					}

					if (rc_client_achievement_get_image_url(curr_achievement,
						RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED, url, sizeof(url)) == RC_OK) {
						std::string path = GetGameCachePath();
						path += fmt::format("/{}_unlocked.png",
							ach_descriptor.name);

						DownloadImage(url, path);
					}
				}

				m_achievements[curr_bucket.label].push_back(std::move(ach_descriptor));
			}
		}

		rc_client_destroy_achievement_list(list);
	}

	std::string RetroAchievements::GetAvatarPath() const {
		return m_cache_path + fmt::format("/{}_avatar.png", m_user.username);
	}

	std::string RetroAchievements::GetGameCachePath() const {
		return m_cache_path + fmt::format("/{}", m_rom_hash);
	}

	std::string RetroAchievements::GetGameBadgePath() const {
		return GetGameCachePath() + "/badge.png";
	}

	void RetroAchievements::ClientIdle() {
		rc_client_idle(m_rc_client);
	}

	void RetroAchievements::ClientReset() {
		rc_client_reset(m_rc_client);
	}

	void RetroAchievements::ClientDoFrame() {
		rc_client_do_frame(m_rc_client);
	}

	void RetroAchievements::ClientEvent(const rc_client_event_t* event) {
		switch (event->type)
		{
		case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
			AchievementTriggered(event->achievement);
			break;
		case RC_CLIENT_EVENT_GAME_COMPLETED:
			AchievedMastery();
			break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
			AchievementProgress(event->achievement);
			break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW:
			AchievementProgressShow(event->achievement);
			break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE:
			break;
		default:
			fmt::println("[ACHIEVEMENTS] Unhandled Event! ({})", event->type);
			break;
		}
	}

	void RetroAchievements::PerformLogin(std::string const& credentials_file) {
		mINI::INIStructure credentials{};

		if (!std::filesystem::exists(credentials_file) ||
			!std::filesystem::is_regular_file(credentials_file)) {
			fmt::println("[ACHIEVEMENTS] Could not read credentials");
			throw std::runtime_error("Invalid credentials");
		}

		{
			mINI::INIFile db_file(credentials_file);
			db_file.read(credentials);
		}
		
		if (!credentials.has("CREDENTIALS")) {
			fmt::println("[ACHIEVEMENTS] Could not read credentials");
			throw std::runtime_error("Invalid credentials");
		}

		if (!credentials["CREDENTIALS"].has("username") ||
			!credentials["CREDENTIALS"].has("password")) {
			fmt::println("[ACHIEVEMENTS] Missing required credentials");
			throw std::runtime_error("Invalid credentials");
		}

		std::string username = credentials["CREDENTIALS"]["username"];
		std::string password = credentials["CREDENTIALS"]["password"];

		bool force_password = false;

		while (true) {
			if (!credentials["CREDENTIALS"].has("token") || force_password) {
				rc_client_begin_login_with_password(m_rc_client,
					username.c_str(), password.c_str(), detail::login_callback,
					this);
				break;
			}
			else {
				std::string token = credentials["CREDENTIALS"]["token"];
				rc_client_begin_login_with_token(m_rc_client,
					username.c_str(), token.c_str(), detail::login_callback,
					this);
				force_password = !IsLoggedIn();

				if (!force_password) {
					break;
				}
			}
		}

		if (IsLoggedIn()) {
			m_user.web_api_key = credentials["CREDENTIALS"]["web_token"];
			m_user.username    = username;
			DownloadImage(m_user.user_internal.avatar_url,
				m_cache_path + fmt::format("/{}_avatar.png", m_user.username));
		}
	}

	void RetroAchievements::DownloadImage(std::string const& url, std::string const& dest) {
		Poco::URI uri{url};

		if (std::filesystem::exists(dest)) {
			Poco::Net::HTTPRequest request{};
			request.setMethod(Poco::Net::HTTPRequest::HTTP_HEAD);
			request.setURI(uri.getPathAndQuery());
			request.setVersion(Poco::Net::HTTPRequest::HTTP_1_1);

			Poco::Net::HTTPClientSession session{};
			session.setHost(uri.getHost());
			session.setPort(80);

			session.setKeepAlive(false);
			session.sendRequest(request);

			Poco::Net::HTTPResponse response{};
			auto& response_stream = session.receiveResponse(response);

			auto status = response.getStatus();

			if (status != Poco::Net::HTTPResponse::HTTP_OK) {
				fmt::println("[ACHIEVEMENTS] Image fetch failed at {}",
					url);
				return;
			}

			auto& last_modified_remote = response.get("Last-Modified");
			std::istringstream is{last_modified_remote};
			std::tm gmtime{};
			is >> std::get_time(&gmtime, "%a, %d %b %Y %H:%M:%S GMT");

			auto last_modified_remote_time = mktime(&gmtime);
			auto last_modified_remote_sys  = 
				std::chrono::system_clock::from_time_t(last_modified_remote_time);

			auto last_modified = std::filesystem::last_write_time(dest);
			auto time_utc      = std::chrono::file_clock::to_utc(last_modified);
			auto time_sys      = std::chrono::utc_clock::to_sys(time_utc);
			
			if (time_sys >= last_modified_remote_sys) {
				fmt::println("[ACHIEVEMENTS] Using cached image {}",
					dest);
				return;
			}
		}

		Poco::Net::HTTPRequest request{};
		request.setMethod(Poco::Net::HTTPRequest::HTTP_GET);
		request.setURI(uri.getPathAndQuery());
		request.setVersion(Poco::Net::HTTPRequest::HTTP_1_1);

		Poco::Net::HTTPClientSession session{};
		session.setHost(uri.getHost());
		session.setPort(80);
		
		session.setKeepAlive(false);
		session.sendRequest(request);

		Poco::Net::HTTPResponse response{};
		auto& response_stream = session.receiveResponse(response);

		auto status = response.getStatus();

		if (status == Poco::Net::HTTPResponse::HTTP_NOT_MODIFIED)
			return;

		if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK) {
			fmt::println("[ACHIEVEMENTS] Image fetch failed at {}",
				url);
			return;
		}

		std::ofstream of{};
		of.open(dest, std::ios::out | std::ios::binary);

		Poco::StreamCopier::copyStream(response_stream, of);
	}

	void RetroAchievements::AchievementTriggered(const rc_client_achievement_t* achievement) {
		auto badge_name = std::string{achievement->badge_name};
		auto ach = GetAchievementByBadge(badge_name);

		if (ach == nullptr) {
			fmt::println("[ACHIEVEMENTS] Could not find achievement with badge {}",
				achievement->badge_name);
			return;
		}

		if (ach->unlocked) {
			fmt::println("[ACHIEVEMENTS] Achievement with badge {} already unlocked",
				achievement->badge_name);
			return;
		}

		fmt::println("[ACHIEVEMENTS] Unlocked \"{}\"", ach->description);
		if (ach->type == RA_AchievementType::UNOFFICIAL) {
			fmt::println("               Unofficial Achievement Unlocked");
		}
		UpdateSummaries();
		LoadAchievementList(false);

		//Old pointer is now invalid, get achievement again
		ach = GetAchievementByBadge(badge_name);
		m_new_unlocked_achievements.push_back(*ach);
	}

	void RetroAchievements::AchievementProgress(const rc_client_achievement_t* achievement) {
		auto badge_name = std::string{achievement->badge_name};
		auto ach = GetAchievementByBadge(badge_name);
		
		if (ach == nullptr) {
			fmt::println("[ACHIEVEMENTS] Could not find achievement with badge {}",
				achievement->badge_name);
			return;
		}

		fmt::println("[ACHIEVEMENTS] Progress for {}, {}",
			ach->name, achievement->measured_progress);

		ach->progress = std::string{achievement->measured_progress};
		m_achievement_progress_update.push_back(*ach);
	}

	void RetroAchievements::AchievementProgressShow(const rc_client_achievement_t* achievement) {
		auto badge_name = std::string{ achievement->badge_name };
		auto ach = GetAchievementByBadge(badge_name);

		if (ach == nullptr) {
			fmt::println("[ACHIEVEMENTS] Could not find achievement with badge {}",
				achievement->badge_name);
			return;
		}

		fmt::println("[ACHIEVEMENTS] Showing progress for {} = {}",
			ach->description, achievement->measured_progress);

		ach->progress = std::string{ achievement->measured_progress };
		m_achievement_progress_update.push_back(*ach);
	}

	void RetroAchievements::AchievedMastery() {
		m_mastery_achieved         = true;
		mastery_achieved_interrupt = true;
	}

	RetroAchievements::RA_Achievement* RetroAchievements::GetAchievementByBadge(std::string const& name)
	{
		for (auto& bucket : m_achievements) {
			auto& [_, list] = bucket;
			for (auto& achievement : list) {
				if (achievement.name == name)
					return &achievement;
			}
		}

		return nullptr;
	}

	void RetroAchievements::UpdateSummaries() {
		rc_client_get_user_game_summary(m_rc_client, &m_game_summary);
		m_user.user_internal = *rc_client_get_user_info(m_rc_client);
	}
}