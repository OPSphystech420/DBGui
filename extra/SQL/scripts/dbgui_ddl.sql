-- MariaDB DBGui DDL
--
-- Database: dbgui
-- ------------------------------------------------------
-- Server version	11.7.2-MariaDB

CREATE DATABASE IF NOT EXISTS dbgui;

-- ===========================================================
-- 1. Users and Roles (unchanged interconnection)
-- ===========================================================
CREATE TABLE dbgui.users (
    user_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    full_name VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE dbgui.roles (
    role_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    role_name VARCHAR(50) NOT NULL UNIQUE,
    description VARCHAR(255)
);

CREATE TABLE dbgui.user_roles (
    user_id INT UNSIGNED NOT NULL,
    role_id INT UNSIGNED NOT NULL,
    PRIMARY KEY (user_id, role_id),
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (role_id) REFERENCES roles(role_id)
);

-- ===========================================================
-- 2. Devices and Database Connections
-- ===========================================================
CREATE TABLE dbgui.user_devices (
    device_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    device_uuid CHAR(36) NOT NULL,
    device_name VARCHAR(100),
    ip_address VARCHAR(45),
    registered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id)
);

CREATE TABLE dbgui.database_connections (
    connection_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    connection_name VARCHAR(100) NOT NULL,
    host VARCHAR(100) NOT NULL,
    port INT UNSIGNED NOT NULL,
    database_name VARCHAR(100) NOT NULL,
    db_username VARCHAR(50),
    db_password VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id)
);

-- ===========================================================
-- 3. Client Logins (SCD Type 2 for connection history)
-- ===========================================================
/*
  The client_logins table is designed with SCD Type 2 versioning.
  - login_time (effective_from) marks the beginning of a session.
  - logout_time (effective_to) is filled in when the user goes offline.
  - is_current indicates whether the record is the current active session.
  - online indicates the connection state (TRUE if online).
  Each state change (login or logout) will result in a new record (or update of effective_to)
  so that the full history of user connection sessions is preserved.
*/
CREATE TABLE dbgui.client_logins (
    login_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    device_id INT UNSIGNED,
    login_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,  -- used as effective_from
    logout_time DATETIME DEFAULT NULL,                        -- used as effective_to when session ends
    is_current BOOLEAN DEFAULT TRUE,                         -- TRUE if session is active
    online BOOLEAN DEFAULT TRUE,                             -- TRUE if online, FALSE if offline
    session_token VARCHAR(100),
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (device_id) REFERENCES user_devices(device_id)
);

-- ===========================================================
-- 4. User Actions (tying sessions and DB connections)
-- ===========================================================
/*
  user_actions captures various events within the application:
  - 'query' for SQL queries.
  - 'setting_change' for changes in user settings.
  - 'background_event' or 'exit' for app state transitions.
  It links to both a login session (client_logins) and an optional database connection.
*/
CREATE TABLE dbgui.user_actions (
    action_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    login_id INT UNSIGNED,              -- links to the client_logins record (SCD)
    connection_id INT UNSIGNED,         -- optionally links to a database connection
    action_type ENUM('query', 'setting_change', 'background_event', 'exit', 'other') NOT NULL,
    description TEXT,
    action_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (login_id) REFERENCES client_logins(login_id),
    FOREIGN KEY (connection_id) REFERENCES database_connections(connection_id)
);

-- ============================================================
-- 5. Error Logs and User Reviews (associated with user actions)
-- ============================================================
/*
  error_logs now records errors in context by linking to a specific user_action,
  helping to pinpoint the cause of an error.
*/
CREATE TABLE dbgui.error_logs (
    error_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED,
    action_id INT UNSIGNED,             -- links to the corresponding action
    error_message TEXT,
    error_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    severity ENUM('low', 'medium', 'high') DEFAULT 'low',
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (action_id) REFERENCES user_actions(action_id)
);

CREATE TABLE dbgui.user_reviews (
    review_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    review_text TEXT NOT NULL,
    review_date DATETIME DEFAULT CURRENT_TIMESTAMP,
    error_id INT UNSIGNED DEFAULT NULL,
    rating TINYINT UNSIGNED NOT NULL CHECK (rating BETWEEN 1 AND 5),
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (error_id) REFERENCES error_logs(error_id)
);

-- ===========================================================
-- 6. User Settings (Logging user-specific changes)
-- ===========================================================
/*
  user_settings logs setting changes made by a user. It links the change to:
  - the user (users),
  - the session during which the change occurred (client_logins),
  - the specific action that triggered it (user_actions).
  A trigger can be implemented later to update program_settings based on the latest
  setting_change action or to reconcile user settings with default settings.
*/
CREATE TABLE dbgui.user_settings (
    user_setting_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    login_id INT UNSIGNED NOT NULL,
    action_id INT UNSIGNED,              -- action that triggered the change
    setting_name VARCHAR(100) NOT NULL,
    setting_value VARCHAR(255) NOT NULL,
    changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (login_id) REFERENCES client_logins(login_id),
    FOREIGN KEY (action_id) REFERENCES user_actions(action_id)
);

-- ===========================================================
-- Triggers and Events
-- ===========================================================
/* Trigger to Update the Online Status When a User Logs Out */
DELIMITER //
CREATE TRIGGER dbgui.before_client_logins_update
BEFORE UPDATE ON dbgui.client_logins
FOR EACH ROW
BEGIN
    IF NEW.logout_time IS NOT NULL THEN
        SET NEW.online = 0;
    ELSE
        SET NEW.online = 1;
    END IF;
END//
DELIMITER ;

/* Event to Mark Inactive Sessions as Offline */
SET GLOBAL event_scheduler = ON;

DELIMITER //
CREATE EVENT dbgui.mark_inactive_sessions
ON SCHEDULE EVERY 5 MINUTE
DO
BEGIN
    UPDATE dbgui.client_logins
    SET online = 0, is_current = 0
    WHERE online = 1 
      AND logout_time IS NULL 
      AND login_time < DATE_SUB(NOW(), INTERVAL 30 MINUTE);
END//
DELIMITER ;