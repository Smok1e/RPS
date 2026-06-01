#pragma once

#include <string>

//========================================

enum class GameMove: uint8_t
{
	Rock,
	Scissors,
	Paper,

	Amount
};

enum class GameResult: uint8_t
{
	Win,
	Defeat,
	Draw
};

GameResult operator<=>(GameMove player_move, GameMove opponent_move);

//========================================

namespace std
{

//========================================

string to_string(GameMove move);
string to_string(GameResult result);

//========================================

} // namespace std

//========================================