CREATE TABLE user (
  id INTEGER PRIMARY KEY,
  username VARCHAR,
  password VARCHAR,
  score INTEGER,
  status SMALLINT 
);

CREATE TABLE game (
  id INTEGER PRIMARY KEY,
  white_id INTEGER,
  black_id INTEGER,
  time_start DATETIME,
  time_end DATETIME,
  winner SMALLINT,
  status SMALLINT,
  FOREIGN KEY (white_id) REFERENCES user(id),
  FOREIGN KEY (black_id) REFERENCES user(id)
);