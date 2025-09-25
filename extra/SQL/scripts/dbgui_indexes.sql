-- MariaDB DBGui Indexes
--
-- Database: dbgui
-- ------------------------------------------------------
-- Server version	11.7.2-MariaDB

-- ====================================================================
-- 1. Quickly find active sessions by user and status
-- ====================================================================
CREATE INDEX idx_client_logins_user_current
	ON dbgui.client_logins (user_id, is_current);

-- ====================================================================
-- 2. Efficiently scan recent logins for timeout/event processing jobs
-- ====================================================================
CREATE INDEX idx_client_logins_login_time
	ON dbgui.client_logins (login_time);