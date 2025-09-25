-- MariaDB DBGui Views
--
-- Database: dbgui
-- ------------------------------------------------------
-- Server version	11.7.2-MariaDB

-- ====================================================================
-- 1. View of all currently active sessions, with user and device info
-- ====================================================================
CREATE OR REPLACE VIEW dbgui.v_current_user_sessions AS
SELECT
    cl.login_id,
    cl.user_id,
    u.username,
    u.full_name,
    cl.device_id,
    ud.device_name,
    cl.login_time,
    cl.logout_time,
    cl.online
FROM
    dbgui.client_logins AS cl
    JOIN dbgui.users AS u ON cl.user_id = u.user_id
    LEFT JOIN dbgui.user_devices AS ud ON cl.device_id = ud.device_id
WHERE
    cl.is_current = TRUE;

-- ====================================================================
-- 2. View of aggregated statistics on user activity
-- ====================================================================
CREATE OR REPLACE VIEW dbgui.v_user_activity_summary AS
SELECT
    u.user_id,
    u.username,
    COUNT(ua.action_id) AS total_actions,
    SUM(CASE WHEN el.error_id IS NOT NULL THEN 1 ELSE 0 END) AS total_errors,
    MAX(ua.action_time) AS last_action_time
FROM
    dbgui.users AS u
    LEFT JOIN dbgui.user_actions AS ua ON u.user_id = ua.user_id
    LEFT JOIN dbgui.error_logs   AS el ON ua.action_id = el.action_id
GROUP BY
    u.user_id,
    u.username;

