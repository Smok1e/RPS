#include <string_view>
#include <print>
#include <cassert>

#include <common/config.hpp>
#include <common/score_provider.hpp>

//========================================

ScoreProvider::ScoreProvider()
{
	// Sqlite3 connection
	check(
		sqlite3_open_v2(
			config::score_db_path.string().c_str(), 
			&m_db,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			nullptr
		)
	);

	// Digest initialization
	m_md_ctx = EVP_MD_CTX_new();
	
	m_md = EVP_MD_fetch(nullptr, config::digest, nullptr);
	assert(EVP_MD_get_size(m_md) == config::digest_size);

	// Creating necessary tables
	execute(
		R"QUERY(
			CREATE TABLE IF NOT EXISTS players(
				id INTEGER PRIMARY KEY,
				username TEXT NOT NULL UNIQUE,
				password_hash BLOB NOT NULL,
				wins INTEGER NOT NULL DEFAULT 0,
				defeats INTEGER NOT NULL DEFAULT 0,
				draws INTEGER NOT NULL DEFAULT 0
			);
		)QUERY"
	);

	// Preparing reusable statements
	m_find_player_statement = prepare(
		R"QUERY(
			SELECT id FROM players WHERE username = ?
		)QUERY"
	);

	m_create_player_statement = prepare(
		R"QUERY(
			INSERT INTO players(username, password_hash) VALUES(?, ?)
		)QUERY"
	);

	m_check_user_password_statement = prepare(
		R"QUERY(
			SELECT EXISTS(SELECT 1 FROM players WHERE id = ? AND password_hash = ?)
		)QUERY"
	);

	m_get_player_score_statement = prepare(
		R"QUERY(
			SELECT wins, defeats, draws FROM players WHERE id = ?
		)QUERY"
	);

	m_set_player_score_statement = prepare(
		R"QUERY(
			UPDATE players SET wins = wins + ?, defeats = defeats + ?, draws = draws + ? WHERE id = ?
		)QUERY"
	);
}

ScoreProvider::~ScoreProvider()
{
	if (m_find_player_statement)
		check(sqlite3_finalize(m_find_player_statement));

	if (m_create_player_statement)
		check(sqlite3_finalize(m_create_player_statement));

	if (m_check_user_password_statement)
		check(sqlite3_finalize(m_check_user_password_statement));

	if (m_get_player_score_statement)
		check(sqlite3_finalize(m_get_player_score_statement));

	if (m_set_player_score_statement)
		check(sqlite3_finalize(m_set_player_score_statement));

	if (m_db)
		check(sqlite3_close(m_db));

	if (m_md_ctx)
		EVP_MD_CTX_free(m_md_ctx);

	if (m_md)
		EVP_MD_free(m_md);
}

//========================================

ScoreProvider::PlayerID ScoreProvider::createPlayer(
	std::string_view username, 
	std::string_view password
)
{
	if (findPlayer(username) != InvalidPlayer)
		return InvalidPlayer;

	EVP_DigestInit(m_md_ctx, m_md);
	EVP_DigestUpdate(m_md_ctx, password.data(), password.size());

	digest_t password_hash;
	unsigned password_hash_len = sizeof(password_hash);
	EVP_DigestFinal(m_md_ctx, password_hash, &password_hash_len);

	check(sqlite3_reset(m_create_player_statement));

	check(
		sqlite3_bind_text(
			m_create_player_statement, 
			1, 
			username.data(), 
			username.length(), 
			SQLITE_STATIC
		)
	);

	check(
		sqlite3_bind_blob(
			m_create_player_statement,
			2,
			password_hash,
			password_hash_len,
			SQLITE_STATIC
		)
	);

	check(sqlite3_step(m_create_player_statement));
	return sqlite3_last_insert_rowid(m_db);
}

ScoreProvider::PlayerID ScoreProvider::findPlayer(std::string_view username)
{
	check(sqlite3_reset(m_find_player_statement));

	check(
		sqlite3_bind_text(
			m_find_player_statement, 
			1, 
			username.data(), 
			username.length(), 
			SQLITE_STATIC
		)
	);

	if (check(sqlite3_step(m_find_player_statement)) != SQLITE_ROW)
		return InvalidPlayer;

	return sqlite3_column_int(m_find_player_statement, 0);
}

bool ScoreProvider::checkPlayerPassword(
	PlayerID player_id, 
	std::string_view password
)
{
	EVP_DigestInit(m_md_ctx, m_md);
	EVP_DigestUpdate(m_md_ctx, password.data(), password.size());

	digest_t password_hash;
	unsigned password_hash_len = sizeof(password_hash);
	EVP_DigestFinal(m_md_ctx, password_hash, &password_hash_len);

	check(sqlite3_reset(m_check_user_password_statement));

	check(
		sqlite3_bind_int(
			m_check_user_password_statement,
			1,
			player_id
		)
	);

	check(
		sqlite3_bind_blob(
			m_check_user_password_statement,
			2,
			password_hash,
			password_hash_len,
			SQLITE_STATIC
		)
	);

	check(sqlite3_step(m_check_user_password_statement));
	return sqlite3_column_int(m_check_user_password_statement, 0);
}

ScoreProvider::PlayerScore ScoreProvider::getPlayerScore(PlayerID player_id)
{
	check(sqlite3_reset(m_get_player_score_statement));

	check(
		sqlite3_bind_int(
			m_get_player_score_statement,
			1,
			player_id
		)
	);

	check(sqlite3_step(m_get_player_score_statement));

	PlayerScore info = {};
	info.wins    = sqlite3_column_int(m_get_player_score_statement, 0);
	info.defeats = sqlite3_column_int(m_get_player_score_statement, 1);
	info.draws   = sqlite3_column_int(m_get_player_score_statement, 2);

	return info;
}

void ScoreProvider::updatePlayerScore(PlayerID player_id, GameResult game_result)
{
	check(sqlite3_reset(m_set_player_score_statement));

	check(sqlite3_bind_int(m_set_player_score_statement, 1, game_result == GameResult::Win   ));
	check(sqlite3_bind_int(m_set_player_score_statement, 2, game_result == GameResult::Defeat));
	check(sqlite3_bind_int(m_set_player_score_statement, 3, game_result == GameResult::Draw  ));
	check(sqlite3_bind_int(m_set_player_score_statement, 4, player_id                        ));

	check(sqlite3_step(m_set_player_score_statement));
}

//========================================

int ScoreProvider::check(int ret)
{
	if (ret == SQLITE_OK || ret == SQLITE_DONE || ret == SQLITE_ROW)
		return ret;

	throw DatabaseError(sqlite3_errmsg(m_db));
}

sqlite3_stmt* ScoreProvider::prepare(std::string_view query)
{
	sqlite3_stmt* statement = nullptr;
	check(
		sqlite3_prepare_v3(
			m_db, 
			query.data(),
			query.size(),
			0,
			&statement,
			nullptr
		)
	);

	return statement;
}

void ScoreProvider::execute(std::string_view query)
{
	auto* statement = prepare(query);
	check(sqlite3_step(statement));
	check(sqlite3_finalize(statement));
}

//========================================

ScoreProvider::PlayerScore& operator+=(ScoreProvider::PlayerScore& lhs, GameResult rhs)
{
	lhs.wins    += (rhs == GameResult::Win   );
	lhs.defeats += (rhs == GameResult::Defeat);
	lhs.draws   += (rhs == GameResult::Draw  );

	return lhs;
}

//========================================