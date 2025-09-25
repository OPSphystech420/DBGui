import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from scipy.stats import ttest_ind, pearsonr, chi2_contingency
from sqlalchemy import create_engine, text

engine = create_engine('mysql+pymysql://test_user:123@localhost/dbgui')
conn = engine.connect()

np.random.seed(42)
n_users = 50
n_devices = 100
n_sessions = 200
n_actions = 500

with conn.begin():
    conn.execute(text("DELETE FROM user_actions"))
    conn.execute(text("DELETE FROM client_logins"))
    conn.execute(text("DELETE FROM user_devices"))
    conn.execute(text("DELETE FROM user_roles"))
    conn.execute(text("DELETE FROM users"))
    conn.execute(text("DELETE FROM roles"))

    conn.execute(text("INSERT INTO roles (role_name, description) VALUES ('user', 'Standard user'), ('admin', 'Administrator')"))
    user_role_id = conn.execute(text("SELECT role_id FROM roles WHERE role_name='user'"))
    user_role_id = user_role_id.fetchone()[0]
    admin_role_id = conn.execute(text("SELECT role_id FROM roles WHERE role_name='admin'"))
    admin_role_id = admin_role_id.fetchone()[0]

    for i in range(n_users):
        conn.execute(
            text("INSERT INTO users (username, email, password, full_name) VALUES (:username, :email, :password, :full_name)"),
            {"username": f'user{i:02d}', "email": f'user{i:02d}@mail.com', "password": 'pass', "full_name": f'User {i:02d}'}
        )
    user_ids = conn.execute(text("SELECT user_id FROM users")).fetchall()
    user_ids = [row[0] for row in user_ids]

    for uid in user_ids:
        conn.execute(text("INSERT INTO user_roles (user_id, role_id) VALUES (:uid, :role_id)"), {"uid": uid, "role_id": user_role_id})
        if np.random.rand() < 0.2:
            conn.execute(text("INSERT INTO user_roles (user_id, role_id) VALUES (:uid, :role_id)"), {"uid": uid, "role_id": admin_role_id})

    for i in range(n_devices):
        uid = np.random.choice(user_ids)
        conn.execute(
            text("INSERT INTO user_devices (user_id, device_uuid, device_name, ip_address) VALUES (:uid, UUID(), :device_name, :ip_address)"),
            {"uid": uid, "device_name": f'Device_{i:02d}', "ip_address": f'192.168.1.{i%50}'}
        )
    device_rows = conn.execute(text("SELECT device_id, user_id FROM user_devices")).fetchall()

    for i in range(n_sessions):
        uid = np.random.choice(user_ids)
        user_devices = [row[0] for row in device_rows if row[1] == uid]
        if not user_devices:
            continue
        did = np.random.choice(user_devices)
        conn.execute(
            text("INSERT INTO client_logins (user_id, device_id, login_time, is_current, online, session_token) VALUES (:uid, :did, NOW() - INTERVAL :hours HOUR, :is_current, :online, UUID())"),
            {"uid": uid, "did": did, "hours": int(np.random.randint(0, 100)), "is_current": int(np.random.choice([0, 1])), "online": int(np.random.choice([0, 1]))}
        )
    login_rows = conn.execute(text("SELECT login_id, user_id FROM client_logins")).fetchall()
    for i in range(n_actions):
        uid = np.random.choice(user_ids)
        user_logins = [row[0] for row in login_rows if row[1] == uid]
        if not user_logins:
            continue
        lid = np.random.choice(user_logins)
        conn.execute(
            text("INSERT INTO user_actions (user_id, login_id, action_type, description) VALUES (:uid, :lid, :action_type, :desc)"),
            {"uid": uid, "lid": lid, "action_type": np.random.choice(['query', 'setting_change', 'background_event', 'exit', 'other']), "desc": f'Action {i}'}
        )

# 3.1 Number of devices per user
df_devices = pd.read_sql("SELECT user_id, COUNT(*) as device_count FROM user_devices GROUP BY user_id", engine)
# 3.2 Number of roles per user
df_roles = pd.read_sql("SELECT user_id, COUNT(*) as role_count FROM user_roles GROUP BY user_id", engine)
# 3.3 Number of sessions per user
df_sessions = pd.read_sql("SELECT user_id, COUNT(*) as session_count FROM client_logins GROUP BY user_id", engine)
# 3.4 Number of actions per user
df_actions = pd.read_sql("SELECT user_id, COUNT(*) as action_count FROM user_actions GROUP BY user_id", engine)

# Combine for analysis
df = df_devices.merge(df_roles, on='user_id', how='outer') \
               .merge(df_sessions, on='user_id', how='outer') \
               .merge(df_actions, on='user_id', how='outer')
df = df.fillna(0)

# 4. Visualization
plt.figure(figsize=(15, 4))
plt.subplot(1, 3, 1)
sns.histplot(df['device_count'], bins=10, kde=True)
plt.title('Распределение числа устройств на пользователя')

plt.subplot(1, 3, 2)
sns.boxplot(x=df['role_count'], y=df['device_count'])
plt.title('Число устройств в зависимости от числа ролей')

plt.subplot(1, 3, 3)
sns.scatterplot(x=df['session_count'], y=df['action_count'])
plt.title('Связь числа сессий и действий')
plt.tight_layout()
plt.show()

# 5. Testing hypotheses

# Hypothesis 1: Users with 2 roles have more devices than those with 1 role
devices_1 = df[df['role_count'] == 1]['device_count']
devices_2 = df[df['role_count'] == 2]['device_count']
t_stat, p_val = ttest_ind(devices_1, devices_2, equal_var=False)
print(f'Гипотеза 1: p-value={p_val:.4f} (среднее устройств: 1 роль={devices_1.mean():.2f}, 2 роли={devices_2.mean():.2f})')

# Hypothesis 2: Is there a correlation between the number of sessions and the number of actions?
corr, p_corr = pearsonr(df['session_count'], df['action_count'])
print(f'Гипотеза 2: Корреляция между сессиями и действиями: r={corr:.2f}, p-value={p_corr:.4f}')

# Hypothesis 3: Is the presence of devices related to the probability of at least one session?
df['has_session'] = df['session_count'] > 0
contingency = pd.crosstab(df['device_count'] > 0, df['has_session'])
chi2, p_chi2, _, _ = chi2_contingency(contingency)
print(f'Гипотеза 3: p-value (chi2)={p_chi2:.4f}')

print("""\nCONCLUSIONS:
1. Distribution of devices per user — see histogram. Most users have 1-2 devices.
2. Users with two roles have on average {‘more’ if devices_2.mean() > devices_1.mean() else ‘less’} devices than those with one role. p-value={p_val:.4f}
3. Correlation between the number of sessions and actions: r={corr:.2f} (p-value={p_corr:.4f}) — {‘yes’ if p_corr < 0.05 else ‘no’} statistically significant relationship.
4. The presence of devices is associated with the probability of at least one session (p-value={p_chi2:.4f}).
""")

conn.close() 
