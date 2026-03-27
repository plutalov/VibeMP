CREATE TABLE accounts (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    username    VARCHAR(24) NOT NULL UNIQUE,
    password    VARCHAR(72) NOT NULL,
    score       INT DEFAULT 0,
    money       INT DEFAULT 0,
    skin        INT DEFAULT 0,
    pos_x       FLOAT DEFAULT -2233.97,
    pos_y       FLOAT DEFAULT -1737.58,
    pos_z       FLOAT DEFAULT 480.55,
    pos_a       FLOAT DEFAULT 0.0,
    health      FLOAT DEFAULT 100.0,
    armor       FLOAT DEFAULT 0.0,
    interior    INT DEFAULT 0,
    vworld      INT DEFAULT 0,
    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_login  DATETIME DEFAULT NULL
);
