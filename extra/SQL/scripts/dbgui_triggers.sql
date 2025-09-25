-- MariaDB DBGui Triggers
--
-- Database: dbgui
-- ------------------------------------------------------
-- Server version	11.7.2-MariaDB

-- ====================================================================
-- 1. Trigger to Update the Online Status When a User Logs Out
-- ====================================================================
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

-- ====================================================================
-- 2. Auto-assign the “user” role to every brand-new account
-- ====================================================================
DELIMITER //
CREATE TRIGGER dbgui.after_users_insert
AFTER INSERT ON dbgui.users
FOR EACH ROW
BEGIN
    DECLARE v_role_id INT;
    SELECT role_id
        INTO v_role_id
        FROM dbgui.roles
    WHERE role_name = 'user'
    LIMIT 1;
    IF v_role_id IS NOT NULL THEN
        INSERT IGNORE INTO dbgui.user_roles(user_id, role_id)
            VALUES (NEW.user_id, v_role_id);
    END IF;
END//
DELIMITER ;

-- ====================================================================
-- 3. Log device registered action whenever someone adds a new device
-- ====================================================================
DELIMITER //
CREATE TRIGGER dbgui.after_user_devices_insert
AFTER INSERT ON dbgui.user_devices
FOR EACH ROW
BEGIN
    DECLARE v_login_id INT;
    SELECT login_id
        INTO v_login_id
        FROM dbgui.client_logins
    WHERE user_id    = NEW.user_id
        AND device_id  = NEW.device_id
        AND is_current = TRUE
    LIMIT 1;
  
    INSERT INTO dbgui.user_actions(
        user_id,
        login_id,
        connection_id,
        action_type,
        description
    ) VALUES (
        NEW.user_id,
        v_login_id,
        NULL,
        'other',
        CONCAT('Registered device ', NEW.device_name, ' (', NEW.device_uuid, ')')
    );
END//
DELIMITER ;
