-- MariaDB DBGui Queries example
--
-- Database: dbgui
-- ------------------------------------------------------
-- Server version	11.7.2-MariaDB

-- ===============================================
-- 1. List of users with more than one role
-- ===============================================
SELECT u.user_id, u.username, COUNT(ur.role_id) AS role_count
FROM dbgui.users u
JOIN dbgui.user_roles ur ON u.user_id = ur.user_id
GROUP BY u.user_id, u.username
HAVING COUNT(ur.role_id) > 1
ORDER BY role_count DESC;

-- ===============================================
-- 2. Online users
-- ===============================================
SELECT user_id, username, email
FROM dbgui.users
WHERE user_id IN (
  SELECT user_id FROM dbgui.client_logins WHERE online = 1
);

-- ===============================================
-- 3. Numbering of sessions by entry time
-- ===============================================
SELECT login_id, user_id, login_time,
       ROW_NUMBER() OVER (PARTITION BY user_id ORDER BY login_time DESC) AS session_rank
FROM dbgui.client_logins;

-- ===============================================
-- 4. Pairs of users registered on the same day
-- ===============================================
SELECT u1.user_id AS user1, u1.username AS username1,
       u2.user_id AS user2, u2.username AS username2,
       DATE(u1.created_at) AS reg_date
FROM dbgui.users u1
JOIN dbgui.users u2 ON DATE(u1.created_at) = DATE(u2.created_at)
  AND u1.user_id < u2.user_id;

-- ===============================================
-- 5. Users with reviews ranked lower than all reviews of 'user01'
-- ===============================================
SELECT DISTINCT ur.user_id, u.username
FROM dbgui.user_reviews ur
JOIN dbgui.users u ON ur.user_id = u.user_id
WHERE ur.rating < ALL (
  SELECT rating FROM dbgui.user_reviews
  WHERE user_id = (SELECT user_id FROM dbgui.users WHERE username = 'user01')
);

-- ===============================================
-- 6. Users with at least one active session
-- ===============================================
SELECT u.user_id, u.username
FROM dbgui.users u
WHERE EXISTS (
  SELECT 1 FROM dbgui.client_logins cl
  WHERE cl.user_id = u.user_id AND cl.online = 1
);

-- ===============================================
-- 7. User list with device information
-- ===============================================
SELECT u.user_id, u.username, d.device_name, d.ip_address
FROM dbgui.users u
LEFT JOIN dbgui.user_devices d ON u.user_id = d.user_id
ORDER BY u.username;

-- ===============================================
-- 8. Interval between user sessions
-- ===============================================
SELECT login_id, user_id, login_time,
       LAG(login_time) OVER (PARTITION BY user_id ORDER BY login_time) AS previous_login,
       TIMESTAMPDIFF(MINUTE,
         LAG(login_time) OVER (PARTITION BY user_id ORDER BY login_time),
         login_time) AS minutes_diff
FROM dbgui.client_logins;

-- ===============================================
-- 9. Last 5 actions by user 'user05'
-- ===============================================
SELECT a.action_id, a.action_type, a.description, a.action_time
FROM dbgui.user_actions a
JOIN dbgui.users u ON a.user_id = u.user_id
WHERE u.username = 'user05'
ORDER BY a.action_time DESC
LIMIT 5 OFFSET 0;

-- ===============================================
-- 10. Users with above or equal average error rates
-- ===============================================
WITH user_error_counts AS (
  SELECT u.user_id, u.username, COUNT(e.error_id) AS total_errors
  FROM dbgui.users u
  JOIN dbgui.error_logs e ON u.user_id = e.user_id
  GROUP BY u.user_id, u.username
)
SELECT user_id, username, total_errors,
       AVG(total_errors) OVER () AS avg_errors_all_users
FROM user_error_counts
WHERE total_errors >= (SELECT AVG(total_errors) FROM user_error_counts)
ORDER BY total_errors DESC;


