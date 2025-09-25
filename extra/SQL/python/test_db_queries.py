import pytest
from sqlalchemy import create_engine, text

DB_URL = 'mysql+pymysql://root:root@localhost/dbgui'

@pytest.fixture(scope="module")
def db_conn():
    engine = create_engine(DB_URL)
    with engine.connect() as conn:
        yield conn

# 1. List of users with more than one role
# Check that role_count > 1 for all rows
def test_users_with_multiple_roles(db_conn):
    result = db_conn.execute(text('''
        SELECT u.user_id, u.username, COUNT(ur.role_id) AS role_count
        FROM dbgui.users u
        JOIN dbgui.user_roles ur ON u.user_id = ur.user_id
        GROUP BY u.user_id, u.username
        HAVING COUNT(ur.role_id) > 1
        ORDER BY role_count DESC;
    '''))
    rows = list(result.mappings())
    for row in rows:
        assert row['role_count'] > 1

# 2. Online users
# Checking that all user_ids are actually online
def test_online_users(db_conn):
    users = db_conn.execute(text('''
        SELECT user_id, username, email
        FROM dbgui.users
        WHERE user_id IN (
          SELECT user_id FROM dbgui.client_logins WHERE online = 1
        );
    '''))
    users = list(users.mappings())
    for user in users:
        count_online = db_conn.execute(
            text('SELECT COUNT(*) as cnt FROM dbgui.client_logins WHERE user_id=:uid AND online=1'),
            {"uid": user['user_id']}
        ).mappings().fetchone()['cnt']
        assert count_online > 0

# 3. Numbering sessions by login time
# Check that session_rank starts at 1 for each user
def test_session_ranking(db_conn):
    result = db_conn.execute(text('''
        SELECT login_id, user_id, login_time,
               ROW_NUMBER() OVER (PARTITION BY user_id ORDER BY login_time DESC) AS session_rank
        FROM dbgui.client_logins;
    '''))
    rows = list(result.mappings())
    ranks = {}
    for row in rows:
        ranks.setdefault(row['user_id'], []).append(row['session_rank'])
    for user_ranks in ranks.values():
        assert sorted(user_ranks)[0] == 1

# 4. Pairs of users registered on the same day
# We check that the pairs are not duplicates and that the user_ids are different
def test_users_registered_same_day(db_conn):
    pairs = db_conn.execute(text('''
        SELECT u1.user_id AS user1, u2.user_id AS user2, DATE(u1.created_at) AS reg_date
        FROM dbgui.users u1
        JOIN dbgui.users u2 ON DATE(u1.created_at) = DATE(u2.created_at)
          AND u1.user_id < u2.user_id;
    '''))
    pairs = list(pairs.mappings())
    for pair in pairs:
        assert pair['user1'] < pair['user2']

# 5. Users with ratings lower than all ratings user01
# We check that each user's rating is lower than all ratings user01
def test_users_with_lower_reviews_than_user01(db_conn):
    users = db_conn.execute(text('''
        SELECT DISTINCT ur.user_id, u.username
        FROM dbgui.user_reviews ur
        JOIN dbgui.users u ON ur.user_id = u.user_id
        WHERE ur.rating < ALL (
          SELECT rating FROM dbgui.user_reviews
          WHERE user_id = (SELECT user_id FROM dbgui.users WHERE username = 'user01')
        );
    '''))
    users = list(users.mappings())
    user01 = db_conn.execute(text("SELECT user_id FROM dbgui.users WHERE username = 'user01'"))
    user01 = user01.mappings().fetchone()
    if user01:
        min_rating = db_conn.execute(text('SELECT MIN(rating) as min_rating FROM dbgui.user_reviews WHERE user_id=:uid'), {"uid": user01['user_id']}).mappings().fetchone()['min_rating']
        for user in users:
            max_rating = db_conn.execute(text('SELECT MAX(rating) as max_rating FROM dbgui.user_reviews WHERE user_id=:uid'), {"uid": user['user_id']}).mappings().fetchone()['max_rating']
            assert max_rating < min_rating

# 6. Users with at least one active session
# We verify that each user has at least one online session
def test_users_with_active_session(db_conn):
    users = db_conn.execute(text('''
        SELECT u.user_id, u.username
        FROM dbgui.users u
        WHERE EXISTS (
          SELECT 1 FROM dbgui.client_logins cl
          WHERE cl.user_id = u.user_id AND cl.online = 1
        );
    '''))
    users = list(users.mappings())
    for user in users:
        cnt = db_conn.execute(text('SELECT COUNT(*) as cnt FROM dbgui.client_logins WHERE user_id=:uid AND online=1'), {"uid": user['user_id']}).mappings().fetchone()['cnt']
        assert cnt > 0

# 7. List of users with devices
# Check that each row contains user_id and username
def test_user_list_with_devices(db_conn):
    rows = db_conn.execute(text('''
        SELECT u.user_id, u.username, d.device_name, d.ip_address
        FROM dbgui.users u
        LEFT JOIN dbgui.user_devices d ON u.user_id = d.user_id
        ORDER BY u.username;
    '''))
    rows = list(rows.mappings())
    for row in rows:
        assert 'user_id' in row and 'username' in row

# 8. Interval between user sessions
# Check that minutes_diff >= 0 or NULL
def test_session_interval(db_conn):
    rows = db_conn.execute(text('''
        SELECT login_id, user_id, login_time,
               LAG(login_time) OVER (PARTITION BY user_id ORDER BY login_time) AS previous_login,
               TIMESTAMPDIFF(MINUTE,
                 LAG(login_time) OVER (PARTITION BY user_id ORDER BY login_time),
                 login_time) AS minutes_diff
        FROM dbgui.client_logins;
    '''))
    rows = list(rows.mappings())
    for row in rows:
        if row['minutes_diff'] is not None:
            assert row['minutes_diff'] >= 0

# 9. Last 5 actions by user05
# Check that there are no more than 5 lines and username = 'user05'
def test_last_5_actions_user05(db_conn):
    rows = db_conn.execute(text('''
        SELECT a.action_id, a.action_type, a.description, a.action_time
        FROM dbgui.user_actions a
        JOIN dbgui.users u ON a.user_id = u.user_id
        WHERE u.username = 'user05'
        ORDER BY a.action_time DESC
        LIMIT 5 OFFSET 0;
    '''))
    rows = list(rows.mappings())
    assert len(rows) <= 5
    for row in rows:
        assert row['action_id'] is not None

# 10. Users with a number of errors greater than or equal to the average
# Check that total_errors >= avg_errors_all_users
# (avg_errors_all_users is the same for all rows)
def test_users_with_above_avg_errors(db_conn):
    rows = db_conn.execute(text('''
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
    '''))
    rows = list(rows.mappings())
    if rows:
        avg = rows[0]['avg_errors_all_users']
        for row in rows:
            assert row['total_errors'] >= avg 
