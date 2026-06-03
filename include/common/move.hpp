#pragma once

#include <string>
#include <cstdint>
#include <utility>

//========================================

/// Game move variants
enum class GameMove: uint8_t
{
	Rock,     //<! Rock move
	Scissors, //<! Scissors move
	Paper,    //<! Paper move

	Amount    //<! GameMove member amount
};

/// Game result enumeration
enum class GameResult: uint8_t
{
	Win,    //!< Win
	Defeat, //!< Defeat
	Draw    //!< Draw
};

/// \brief Game move comparison operator
/// 
/// \param player_move Left hand side operand
/// \param opponent_move Right hand side operand
/// \return Game result
/// 
/// Returns GameResult based on left hand side operand (player move)
/// and right hand side operand (opponent move). Returns GameResult::Win if player 
/// beats the opponent, GameResult::Defeat othervise, or GameResult::Draw if the choises
/// are equal.
GameResult operator<=>(GameMove player_move, GameMove opponent_move);

//========================================

namespace std
{

//========================================

/// \brief GameMove overload for std::to_string
/// 
/// \param move GameMove instance
/// \return GameMove instance string representation
string to_string(GameMove move);

/// GameResult overload for std::to_string
/// \param result GameResult instance
/// \return GameResult instance string representation
string to_string(GameResult result);

//========================================

} // namespace std

//========================================