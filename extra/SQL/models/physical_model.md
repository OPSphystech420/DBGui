# Physical Model

## **client_logins**

| Name            | Description                                         | Data Type          | Constraints                                                                 |
|-----------------|-----------------------------------------------------|--------------------|-----------------------------------------------------------------------------|
| **login_id**    | Login record identifier (**PK**)                    | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                    |
| **user_id**     | Reference to user (**FK** → `users.user_id`)        | int(10) unsigned   | NOT NULL                                                                    |
| **device_id**   | Reference to device (**FK** → `user_devices.device_id`) | int(10) unsigned | DEFAULT NULL                                                                |
| **login_time**  | Login time                                          | datetime           | NOT NULL, DEFAULT current_timestamp()                                       |
| **logout_time** | Logout time                                         | datetime           | DEFAULT NULL                                                                |
| **is_current**  | Flag for current active session                     | tinyint(1)         | DEFAULT 1                                                                   |
| **online**      | Online (1) or offline (0) flag                      | tinyint(1)         | DEFAULT 1                                                                   |
| **session_token** | Session token                                     | varchar(100)       | DEFAULT NULL                                                                |

---

## **database_connections**

| Name              | Description                                              | Data Type          | Constraints                                                                 |
|-------------------|----------------------------------------------------------|--------------------|-----------------------------------------------------------------------------|
| **connection_id** | Connection identifier (**PK**)                           | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                    |
| **user_id**       | Reference to user (**FK** → `users.user_id`)             | int(10) unsigned   | NOT NULL                                                                    |
| **connection_name**| Connection name                                         | varchar(100)       | NOT NULL                                                                    |
| **host**          | Host/server address                                      | varchar(100)       | NOT NULL                                                                    |
| **port**          | Server port                                              | int(10) unsigned   | NOT NULL                                                                    |
| **database_name** | Database name                                            | varchar(100)       | NOT NULL                                                                    |
| **db_username**   | Username for DB connection                               | varchar(50)        | DEFAULT NULL                                                                |
| **db_password**   | Password for DB connection                               | varchar(255)       | DEFAULT NULL                                                                |
| **created_at**    | Record creation time                                     | timestamp          | DEFAULT current_timestamp()                                                 |

---

## **error_logs**

| Name             | Description                                               | Data Type          | Constraints                                                                 |
|------------------|-----------------------------------------------------------|--------------------|-----------------------------------------------------------------------------|
| **error_id**     | Error identifier (**PK**)                                 | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                    |
| **user_id**      | Reference to user (**FK** → `users.user_id`)              | int(10) unsigned   | DEFAULT NULL                                                                |
| **action_id**    | Reference to action (**FK** → `user_actions.action_id`)   | int(10) unsigned   | DEFAULT NULL                                                                |
| **error_message**| Error message                                             | text               | DEFAULT NULL                                                                |
| **error_time**   | Time of error occurrence                                  | timestamp          | DEFAULT current_timestamp()                                                 |
| **severity**     | Error severity level (low, medium, high)                  | enum(...)          | DEFAULT 'low'                                                               |

---

## **roles**

| Name          | Description                                    | Data Type        | Constraints                               |
|---------------|------------------------------------------------|------------------|-------------------------------------------|
| **role_id**   | Role identifier (**PK**)                       | int(10) unsigned | NOT NULL, AUTO_INCREMENT                  |
| **role_name** | Role name (**UK**)                             | varchar(50)      | NOT NULL, UNIQUE                          |
| **description**| Role description                              | varchar(255)     | DEFAULT NULL                              |

*(Predefined data: admin, user)*

---

## **user_actions**

| Name             | Description                                                   | Data Type          | Constraints                                                                 |
|------------------|---------------------------------------------------------------|--------------------|-----------------------------------------------------------------------------|
| **action_id**    | Action identifier (**PK**)                                    | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                    |
| **user_id**      | Reference to user (**FK** → `users.user_id`)                  | int(10) unsigned   | NOT NULL                                                                    |
| **login_id**     | Reference to login record (**FK** → `client_logins.login_id`) | int(10) unsigned   | DEFAULT NULL                                                                |
| **connection_id**| Reference to DB connection (**FK** → `database_connections.connection_id`) | int(10) unsigned | DEFAULT NULL                                                                |
| **action_type**  | Action type (query, setting_change, background_event, exit, other) | enum(...)        | NOT NULL                                                                    |
| **description**  | Action description                                            | text               | DEFAULT NULL                                                                |
| **action_time**  | Action execution time                                         | timestamp          | DEFAULT current_timestamp()                                                 |

---

## **user_devices**

| Name             | Description                                     | Data Type          | Constraints                              |
|------------------|-------------------------------------------------|--------------------|------------------------------------------|
| **device_id**    | Device identifier (**PK**)                      | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                 |
| **user_id**      | Reference to user (**FK** → `users.user_id`)    | int(10) unsigned   | NOT NULL                                 |
| **device_uuid**  | Unique device identifier                        | char(36)           | NOT NULL                                 |
| **device_name**  | Device name (description)                       | varchar(100)       | DEFAULT NULL                             |
| **ip_address**   | Device IP address                               | varchar(45)        | DEFAULT NULL                             |
| **registered_at**| Device registration time                        | timestamp          | DEFAULT current_timestamp()              |

---

## **user_reviews**

| Name           | Description                                         | Data Type          | Constraints                                                                 |
|----------------|-----------------------------------------------------|--------------------|-----------------------------------------------------------------------------|
| **review_id**  | Review identifier (**PK**)                          | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                    |
| **user_id**    | Reference to user (**FK** → `users.user_id`)        | int(10) unsigned   | NOT NULL                                                                    |
| **review_text**| Review text                                         | text               | NOT NULL                                                                    |
| **review_date**| Review creation date/time                           | datetime           | DEFAULT current_timestamp()                                                 |
| **error_id**   | Reference to error (**FK** → `error_logs.error_id`) | int(10) unsigned   | DEFAULT NULL                                                                |
| **rating**     | Rating (1–5)                                        | tinyint(3) unsigned| NOT NULL, CHECK (`rating` BETWEEN 1 AND 5)                                  |

---

## **user_roles**

| Name        | Description                                              | Data Type        | Constraints                              |
|-------------|----------------------------------------------------------|------------------|------------------------------------------|
| **user_id** | Reference to user (**PK**, **FK** → `users.user_id`)     | int(10) unsigned | NOT NULL                                 |
| **role_id** | Reference to role (**PK**, **FK** → `roles.role_id`)     | int(10) unsigned | NOT NULL                                 |

*(Composite primary key on (user_id, role_id))*  

---

## **user_settings**

| Name              | Description                                              | Data Type          | Constraints                                                                 |
|-------------------|----------------------------------------------------------|--------------------|-----------------------------------------------------------------------------|
| **user_setting_id**| User setting identifier (**PK**)                        | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                    |
| **user_id**       | Reference to user (**FK** → `users.user_id`)             | int(10) unsigned   | NOT NULL                                                                    |
| **login_id**      | Reference to login record (**FK** → `client_logins.login_id`) | int(10) unsigned | NOT NULL                                                                    |
| **action_id**     | Reference to action (**FK** → `user_actions.action_id`)  | int(10) unsigned   | DEFAULT NULL                                                                |
| **setting_name**  | Setting name                                             | varchar(100)       | NOT NULL                                                                    |
| **setting_value** | Setting value                                            | varchar(255)       | NOT NULL                                                                    |
| **changed_at**    | Setting change time                                      | timestamp          | DEFAULT current_timestamp()                                                 |

---

## **users**

| Name          | Description                                | Data Type          | Constraints                              |
|---------------|--------------------------------------------|--------------------|------------------------------------------|
| **user_id**   | User identifier (**PK**)                   | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                 |
| **username**  | Username (**UK**)                          | varchar(50)        | NOT NULL, UNIQUE                         |
| **email**     | User email (**UK**)                        | varchar(100)       | NOT NULL, UNIQUE                         |
| **password**  | Password (hash)                            | varchar(255)       | NOT NULL                                 |
| **full_name** | User full name                             | varchar(100)       | DEFAULT NULL                             |
| **created_at**| Record creation date/time                  | timestamp          | DEFAULT current_timestamp()              |

---
