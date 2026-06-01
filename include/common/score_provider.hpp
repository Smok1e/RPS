#pragma once

#include <stdexcept>

#include <sqlite3.h>
#include <openssl/evp.h>

#include <common/config.hpp>
#include <common/move.hpp>

//========================================

class DatabaseError: public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

//========================================

class ScoreProvider
{
public:
	using PlayerID = int;
	static const PlayerID InvalidPlayer = -1;

	struct PlayerScore
	{
		unsigned wins = 0; 
		unsigned defeats = 0;
		unsigned draws = 0;
	};

	ScoreProvider();
	~ScoreProvider();

	PlayerID createPlayer(std::string_view username, std::string_view password);
	PlayerID findPlayer(std::string_view username);
	bool checkPlayerPassword(PlayerID player_id, std::string_view password);
	PlayerScore getPlayerScore(PlayerID player_id);
	void updatePlayerScore(PlayerID player_id, GameResult game_result);

private:
	using digest_t = uint8_t[config::digest_size];

	EVP_MD_CTX* m_md_ctx = nullptr;
	EVP_MD* m_md = nullptr;

	sqlite3* m_db = nullptr;
	sqlite3_stmt* m_create_player_statement = nullptr;
	sqlite3_stmt* m_find_player_statement = nullptr;
	sqlite3_stmt* m_check_user_password_statement = nullptr;
	sqlite3_stmt* m_get_player_score_statement = nullptr;
	sqlite3_stmt* m_set_player_score_statement = nullptr;

	int check(int ret);
	sqlite3_stmt* prepare(std::string_view query);
	void execute(std::string_view query);

};

//========================================

ScoreProvider::PlayerScore& operator+=(ScoreProvider::PlayerScore& lhs, GameResult rhs);

//========================================