CREATE TABLE vehicles (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    model_id    INT NOT NULL,
    pos_x       FLOAT NOT NULL DEFAULT 0.0,
    pos_y       FLOAT NOT NULL DEFAULT 0.0,
    pos_z       FLOAT NOT NULL DEFAULT 0.0,
    pos_a       FLOAT NOT NULL DEFAULT 0.0,
    color1      INT NOT NULL DEFAULT -1,
    color2      INT NOT NULL DEFAULT -1,
    health      FLOAT NOT NULL DEFAULT 1000.0,
    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP
);
