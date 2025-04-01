#pragma once

#include "../common/Defs.hpp"

#include "../rcheevos/include/rcheevos.h"
#include "../rcheevos/include/rc_client.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace GBA::emulation {
	using namespace common;

	class Emulator;

	class RetroAchievements {
	public :
		RetroAchievements(Emulator* emu, std::string const& credentials_file);
		~RetroAchievements();

		void SetLoggedIn() {
			m_logged_in = true;
		}

		void SetGameLoaded() {
			m_game_loaded = true;
		}
		
		inline bool IsLoggedIn() const {
			return m_logged_in;
		}

		inline bool IsGameLoaded() const {
			return m_game_loaded;
		}

		void FillUserInfo(rc_client_user_t const* client_user);

		struct RA_User {
			std::string username      = {};
			std::string connect_token = {};
			std::string web_api_key   = {};
			rc_client_user_t user_internal = {};
		};

		enum class RA_AchievementType {
			CORE,
			UNOFFICIAL,
			UNSUPPORTED
		};

		struct RA_Achievement {
			std::string name           = {};
			std::string description    = {};
			std::string bucket_label   = {};
			bool unlocked		       = false;
			std::string image_locked   = {};
			std::string image_unlocked = {};
			RA_AchievementType type    = RA_AchievementType::CORE;
			std::string progress       = {};
		};

		void LoadGame(u8* rom_base, std::size_t rom_size);
		void LoadGameInfo();
		void LoadAchievementList(bool download_badges);

		inline RA_User const& GetUser() const {
			return m_user;
		}

		std::string GetAvatarPath()    const;
		std::string GetGameCachePath() const;
		std::string GetGameBadgePath() const;

		inline const rc_client_game_t* GetGameInfo() const {
			return m_game_info;
		}

		inline auto const& GetUserGameSummary() const {
			return m_game_summary;
		}

		inline auto const& GetAchievements() const {
			return m_achievements;
		}

		inline Emulator* GetEmulator() const {
			return m_emu;
		}

		void ClientIdle();
		void ClientReset();
		void ClientDoFrame();

		void ClientEvent(const rc_client_event_t* event);

		auto& GetRecentlyUnlocked() {
			return m_new_unlocked_achievements;
		}

		void ClearRecentlyUnlocked() {
			m_new_unlocked_achievements.clear();
		}

		auto& GetProgressUpdates() {
			return m_achievement_progress_update;
		}

		void ClearProgressUpdates() {
			m_achievement_progress_update.clear();
		}

		bool mastery_achieved_interrupt;

	private :
		void PerformLogin(std::string const& credentials_file);
		void DownloadImage(std::string const& url, std::string const& dest);

		void AchievementTriggered(const rc_client_achievement_t* achievement);
		void AchievementProgress(const rc_client_achievement_t* achievement);
		void AchievementProgressShow(const rc_client_achievement_t* achievement);
		void AchievedMastery();

		RA_Achievement* GetAchievementByBadge(std::string const& name);

		void UpdateSummaries();

	private :
		Emulator*    m_emu;
		rc_client_t* m_rc_client;
		bool         m_logged_in;
		RA_User      m_user;
		std::string  m_rom_hash;
		bool         m_game_loaded;
		std::string  m_cache_path;
		const rc_client_game_t*       m_game_info;
		rc_client_user_game_summary_t m_game_summary{};
		std::unordered_map<std::string, std::vector<RA_Achievement>>  m_achievements;
		std::vector<RA_Achievement> m_new_unlocked_achievements;
		bool m_mastery_achieved;
		std::vector<RA_Achievement> m_achievement_progress_update;
	};
}