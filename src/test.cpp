#include <print>
#include <iostream>

#include <common/score_provider.hpp>
#include <common/move.hpp>

//========================================

int main()
{
	#define test(expr) std::println("{}: {}", #expr, std::to_string(expr))

	test(GameMove::Rock     <=> GameMove::Rock    );
	test(GameMove::Rock     <=> GameMove::Scissors);
	test(GameMove::Rock     <=> GameMove::Paper   );
	test(GameMove::Scissors <=> GameMove::Rock    );
	test(GameMove::Scissors <=> GameMove::Scissors);
	test(GameMove::Scissors <=> GameMove::Paper   );
	test(GameMove::Paper    <=> GameMove::Rock    );
	test(GameMove::Paper    <=> GameMove::Scissors);
	test(GameMove::Paper    <=> GameMove::Paper   );
}

//========================================