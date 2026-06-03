#pragma once

#include <stdexcept>

#include <sqlite3.h>
#include <openssl/evp.h>

#include <common/config.hpp>
#include <common/move.hpp>

//========================================

/// Generic database error
class DatabaseError: public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

//========================================

/// \brief The class for managing players and their score
///
/// This class basically wraps underlying sqlite database calls
/// in order to provide convenient interface for creating, querying
/// and modifying player assosiated records.
/// 
/// \see config
class ScoreProvider
{
public:
	/// Player id type alias
	using PlayerID = int;

	/// \brief Invalid player constant
	///
	/// This value indicates that the requested player
	/// was not found or could not be created
	static const PlayerID InvalidPlayer = -1;

	/// A structure to hold player score
	struct PlayerScore
	{
		unsigned wins = 0;    // !< The number of wins
		unsigned defeats = 0; // !< The number of defeats
		unsigned draws = 0;	  // !< The number of draws
	};

	ScoreProvider();  //!< Default constructor
	~ScoreProvider(); //!< Destructor

	/// \brief Creates new player entry
	///
	/// \param username The name to assign to player
	/// \param password The password to assign to player
	/// \return Created player id or InvalidPlayer
	/// 
	/// Attempts to create a player entry with the specified username and password.
	/// The password is stored as its hash, according to the digest algorithm specified
	/// in config. In case of player creation failure, e. g. the requested username is
	/// already occupied, the return value is InvalidPlayer.
	/// 
	/// \see config, InvalidPlayer
	PlayerID createPlayer(std::string_view username, std::string_view password);

	/// \brief Returns player id by username
	///
	/// \param username Requested player name
	/// \returns Requested player id or InvalidPlayer
	/// 
	/// Attempts to find requested player by its name and returns its id.
	/// In case the player is not found, InvalidPlayer is returned.
	/// 
	/// \see InvalidPlayer
	PlayerID findPlayer(std::string_view username);

	/// \brief Checks whether the password is correct for the specified player
	///
	/// \param player_id ID of the player to check password for
	/// \param password Password to check
	/// \return Password validation result
	/// 
	/// The method computes given password hash, according to the digest algorithm
	/// specified in config, and compares it with the database record.
	/// 
	/// \see PlayerID, config
	bool checkPlayerPassword(PlayerID player_id, std::string_view password);

	/// \brief Returns player score record
	///
	/// \param player_id The player whose score shold be returned
	/// \return The player score
	/// 
	/// \see PlayerID
	PlayerScore getPlayerScore(PlayerID player_id);

	/// \brief Updates player score
	///
	/// \param player_id The player whose score needs to be updated
	/// \param game_result The result of the game
	/// 
	/// The method updates player score by incrementing corresponding
	/// record in the database according to game result.
	/// 
	/// \see PlayerID, GameResult
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

/// \brief Appends game result to the player score instance
///
/// \param lhs The player score instance reference
/// \param rgs The game result
/// 
/// The method updates player score by incrementing corresponding
/// field according to the right hand side operand.
/// 
/// \see PlayerScore, GameResult
ScoreProvider::PlayerScore& operator+=(ScoreProvider::PlayerScore& lhs, GameResult rhs);

//========================================