-- MariaDB DBGui Functions
--
-- Database: dbgui
-- ------------------------------------------------------
-- Server version	11.7.2-MariaDB

-- ====================================================================
-- 1. User registration and role
-- ====================================================================
DELIMITER //

DROP PROCEDURE IF EXISTS dbgui.sp_user_register//
CREATE PROCEDURE dbgui.sp_user_register(
  IN p_username VARCHAR(50),
  IN p_email VARCHAR(100),
  IN p_password VARCHAR(255),
  IN p_full_name VARCHAR(100)
)
main: BEGIN
  DECLARE v_user_id INT;
  DECLARE v_role_id INT;

  DECLARE EXIT HANDLER FOR SQLEXCEPTION
  BEGIN
    ROLLBACK;
    SELECT 0 AS user_id, 'internal_error' AS error;
  END;

  IF EXISTS (SELECT 1 FROM dbgui.users WHERE username = p_username) THEN
    SELECT 0 AS user_id, 'username_exists' AS error;
    LEAVE main;
  END IF;

  IF EXISTS (SELECT 1 FROM dbgui.users WHERE email = p_email) THEN
    SELECT 0 AS user_id, 'email_exists' AS error;
    LEAVE main;
  END IF;

  START TRANSACTION;
    INSERT INTO dbgui.users(username, email, password, full_name)
    VALUES(p_username, p_email, p_password, p_full_name);

    SET v_user_id = LAST_INSERT_ID();

    SELECT role_id INTO v_role_id
    FROM dbgui.roles
    WHERE role_name = 'undefined_entity'
    LIMIT 1;

    IF v_role_id IS NULL THEN
      INSERT INTO dbgui.roles(role_name, description)
      VALUES('undefined_entity', 'Role is not set');
      SET v_role_id = LAST_INSERT_ID();
    END IF;

    INSERT INTO dbgui.user_roles(user_id, role_id)
    VALUES(v_user_id, v_role_id);
  COMMIT;

  SELECT v_user_id AS user_id, 'ok' AS status;
END//
DELIMITER ;

-- ====================================================================
-- 2. User login and credential check
-- ====================================================================
DELIMITER //

DROP PROCEDURE IF EXISTS dbgui.sp_add_device//
CREATE PROCEDURE dbgui.sp_add_device(
  IN p_user_id INT,
  IN p_device_name VARCHAR(100),
  IN p_ip_address VARCHAR(45),
  OUT p_device_id INT,
  OUT p_device_uuid CHAR(36)
)
BEGIN
  SET p_device_uuid = UUID();
  INSERT INTO user_devices(user_id, device_uuid, device_name, ip_address)
  VALUES(p_user_id, p_device_uuid, p_device_name, p_ip_address);
  SET p_device_id = LAST_INSERT_ID();
END//

DROP FUNCTION IF EXISTS dbgui.fn_validate_user//
CREATE FUNCTION dbgui.fn_validate_user(
  p_username VARCHAR(50),
  p_password VARCHAR(255)
) RETURNS INT DETERMINISTIC
BEGIN
  DECLARE v_id INT;
  SELECT user_id INTO v_id
  FROM users
  WHERE username = p_username
    AND password = p_password;
  RETURN IFNULL(v_id, 0);
END//

DROP PROCEDURE IF EXISTS dbgui.sp_user_login//
CREATE PROCEDURE dbgui.sp_user_login(
  IN p_username VARCHAR(50),
  IN p_password VARCHAR(255),
  IN p_device_name VARCHAR(100),
  IN p_ip_address VARCHAR(45)
)
proc: BEGIN
  DECLARE v_user_id INT DEFAULT 0;
  DECLARE v_role_id INT;
  DECLARE v_device_id INT;
  DECLARE v_device_uuid CHAR(36);
  DECLARE v_login_id INT;

  DECLARE EXIT HANDLER FOR SQLEXCEPTION
  BEGIN
    ROLLBACK;
    SELECT 0 AS user_id, NULL AS role_id, NULL AS device_id,
           NULL AS device_uuid, NULL AS login_id;
  END;

  SET v_user_id = dbgui.fn_validate_user(p_username, p_password);
  IF v_user_id = 0 THEN
    SELECT 0 AS user_id, NULL AS role_id, NULL AS device_id,
           NULL AS device_uuid, NULL AS login_id;
    LEAVE proc;
  END IF;

  SELECT role_id INTO v_role_id
  FROM dbgui.user_roles
  WHERE user_id = v_user_id
  ORDER BY role_id
  LIMIT 1;

  START TRANSACTION;
    CALL dbgui.sp_add_device(
      v_user_id, p_device_name, p_ip_address,
      v_device_id, v_device_uuid
    );

    CALL dbgui.sp_start_session(
      v_user_id, v_device_id, UUID(),
      v_login_id
    );
  COMMIT;

  SELECT v_user_id AS user_id, v_role_id AS role_id,
         v_device_id AS device_id, v_device_uuid AS device_uuid,
         v_login_id AS login_id;
END//

DELIMITER ;

-- ====================================================================
-- 3. Session (SCD Type 2) lifecycle
-- ====================================================================
DELIMITER //

CREATE PROCEDURE dbgui.sp_start_session(
  IN p_user_id INT,
  IN p_device_id INT,
  IN p_session_token VARCHAR(100),
  OUT p_login_id INT
)
BEGIN
  START TRANSACTION;
    UPDATE client_logins
    SET logout_time = NOW(), is_current = FALSE, online = FALSE
    WHERE user_id = p_user_id AND is_current = TRUE;

    INSERT INTO client_logins(user_id, device_id, session_token)
    VALUES(p_user_id, p_device_id, p_session_token);
    SET p_login_id = LAST_INSERT_ID();
  COMMIT;
END//

CREATE PROCEDURE dbgui.sp_end_session(
  IN p_login_id INT
)
BEGIN
  UPDATE client_logins
  SET logout_time = NOW(), is_current = FALSE, online = FALSE
  WHERE login_id = p_login_id AND is_current = TRUE;
END//
DELIMITER ;

-- ====================================================================
-- 4. Action and error logging
-- ====================================================================
DELIMITER //

CREATE PROCEDURE dbgui.sp_log_action(
  IN p_user_id INT,
  IN p_login_id INT,
  IN p_connection_id INT,
  IN p_action_type ENUM('query','setting_change','background_event','exit','other'),
  IN p_description TEXT,
  OUT p_action_id INT
)
BEGIN
  INSERT INTO user_actions(user_id, login_id, connection_id, action_type, description)
  VALUES(p_user_id, p_login_id, p_connection_id, p_action_type, p_description);
  SET p_action_id = LAST_INSERT_ID();
END//

CREATE PROCEDURE dbgui.sp_add_connection(
  IN p_user_id INT,
  IN p_connection_name VARCHAR(100),
  IN p_host VARCHAR(100),
  IN p_port INT,
  IN p_database_name VARCHAR(100),
  IN p_db_username VARCHAR(50),
  IN p_db_password VARCHAR(255),
  OUT p_connection_id INT
)
BEGIN
  INSERT INTO database_connections(
    user_id, connection_name, host, port, database_name, db_username, db_password
  )
  VALUES(
    p_user_id, p_connection_name, p_host, p_port, p_database_name, p_db_username, p_db_password
  );
  SET p_connection_id = LAST_INSERT_ID();
END//

CREATE PROCEDURE dbgui.sp_log_error(
  IN p_user_id INT,
  IN p_action_id INT,
  IN p_error_message TEXT,
  IN p_severity ENUM('low','medium','high'),
  OUT p_error_id INT
)
BEGIN
  INSERT INTO error_logs(user_id, action_id, error_message, severity)
  VALUES(p_user_id, p_action_id, p_error_message, p_severity);
  SET p_error_id = LAST_INSERT_ID();
END//
DELIMITER ;

-- ====================================================================
-- 5. Reviews and Settings
-- ====================================================================
DELIMITER //

CREATE PROCEDURE dbgui.sp_add_review(
  IN p_user_id INT,
  IN p_review_text TEXT,
  IN p_rating TINYINT UNSIGNED,
  IN p_error_id INT,
  OUT p_review_id INT
)
BEGIN
  INSERT INTO user_reviews(user_id, review_text, rating, error_id)
  VALUES(p_user_id, p_review_text, p_rating, p_error_id);
  SET p_review_id = LAST_INSERT_ID();
END//

CREATE PROCEDURE dbgui.sp_change_setting(
  IN p_user_id INT,
  IN p_login_id INT,
  IN p_action_id INT,
  IN p_setting_name VARCHAR(100),
  IN p_setting_value VARCHAR(255),
  OUT p_user_setting_id INT
)
BEGIN
  INSERT INTO user_settings(user_id, login_id, action_id, setting_name, setting_value)
  VALUES(p_user_id, p_login_id, p_action_id, p_setting_name, p_setting_value);
  SET p_user_setting_id = LAST_INSERT_ID();
END//
DELIMITER ;
