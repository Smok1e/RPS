#include <common/move.hpp>

//========================================

GameResult operator<=>(GameMove player_move, GameMove opponent_move)
{
	if (player_move == opponent_move)
		return GameResult::Draw;

	if (
		opponent_move == static_cast<GameMove>((std::to_underlying(player_move) + 1) % 
		std::to_underlying(GameMove::Amount))
	)
		return GameResult::Win;

	return GameResult::Defeat;
}

//========================================

namespace std
{

//========================================

string to_string(GameMove move)
{
	switch (move)
	{
		case GameMove::Rock:     return "rock";
		case GameMove::Paper:    return "paper";
		case GameMove::Scissors: return "scissors";

		default:
			return "your fat mom";
	}
}

string to_string(GameResult result)
{
	switch (result)
	{
		case GameResult::Win:    return "win";
		case GameResult::Defeat: return "defeat";
		case GameResult::Draw:   return "draw";
	}
}

//========================================

} // namespace std

//========================================