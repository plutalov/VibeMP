-- Item templates: define all possible item types
CREATE TABLE item_templates (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    name            VARCHAR(32) NOT NULL,
    category        VARCHAR(16) NOT NULL,
    weight          FLOAT NOT NULL DEFAULT 1.0,
    max_stack       INT NOT NULL DEFAULT 1,
    model_id        INT NOT NULL DEFAULT 0,
    default_metadata VARCHAR(128) DEFAULT ''
);

-- Containers: universal storage abstraction
CREATE TABLE containers (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    owner_type      ENUM('player','vehicle','property','world') NOT NULL,
    owner_id        INT NOT NULL,
    max_weight      FLOAT NOT NULL DEFAULT 30.0,
    created_at      DATETIME DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_owner (owner_type, owner_id)
);

-- Container items: actual item instances in containers
CREATE TABLE container_items (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    container_id    INT NOT NULL,
    slot            INT NOT NULL DEFAULT -1,
    template_id     INT NOT NULL,
    quantity        INT NOT NULL DEFAULT 1,
    metadata        VARCHAR(128) DEFAULT '',
    FOREIGN KEY (container_id) REFERENCES containers(id) ON DELETE CASCADE,
    FOREIGN KEY (template_id) REFERENCES item_templates(id),
    INDEX idx_container (container_id)
);

-- Seed: starter items for testing
INSERT INTO item_templates (name, category, weight, max_stack, model_id) VALUES
    ('Bandage',  'medical',    0.2, 20, 11738),
    ('Phone',    'electronic', 0.3, 1,  18868);
