CREATE TABLE user (
  id INTEGER PRIMARY KEY,
  username VARCHAR,
  password VARCHAR,
  score INTEGER,
);

CREATE TABLE game (
  id INTEGER PRIMARY KEY,
  white_id INTEGER,
  black_id INTEGER,
  time_start DATETIME,
  time_end DATETIME,
  winner SMALLINT,
  FOREIGN KEY (white_id) REFERENCES user(id),
  FOREIGN KEY (black_id) REFERENCES user(id)
);
SELECT
    game.id AS game_id,
    white_user.username AS white_username,
    black_user.username AS black_username,
    CASE
        WHEN game.winner = 1 then 'WIN'
        WHEN game.winner = 0 then 'DRAW'
        else 'LOSS'
    END AS winner_username
FROM
    game
JOIN
    user AS white_user ON game.white_id = white_user.id
JOIN
    user AS black_user ON game.black_id = black_user.id;
