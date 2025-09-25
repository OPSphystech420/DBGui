# Physical Model

## **client_logins**

| Название       | Описание                                           | Тип данных         | Ограничения                                                                                  |
|----------------|----------------------------------------------------|--------------------|----------------------------------------------------------------------------------------------|
| **login_id**   | Идентификатор записи входа (**PK**)                | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                                     |
| **user_id**    | Ссылка на пользователя (**FK** → `users.user_id`)  | int(10) unsigned   | NOT NULL                                                                                    |
| **device_id**  | Ссылка на устройство (**FK** → `user_devices.device_id`) | int(10) unsigned   | DEFAULT NULL                                                                                |
| **login_time** | Время входа                                        | datetime           | NOT NULL, DEFAULT current_timestamp()                                                        |
| **logout_time**| Время выхода                                       | datetime           | DEFAULT NULL                                                                                |
| **is_current** | Признак текущей активной сессии                    | tinyint(1)         | DEFAULT 1                                                                                    |
| **online**     | Признак «онлайн» (1) или «оффлайн» (0)             | tinyint(1)         | DEFAULT 1                                                                                    |
| **session_token** | Токен сессии                                    | varchar(100)       | DEFAULT NULL                                                                                 |

---

## **database_connections**

| Название           | Описание                                                  | Тип данных         | Ограничения                                                           |
|--------------------|-----------------------------------------------------------|--------------------|-----------------------------------------------------------------------|
| **connection_id**  | Идентификатор подключения (**PK**)                        | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                              |
| **user_id**        | Ссылка на пользователя (**FK** → `users.user_id`)         | int(10) unsigned   | NOT NULL                                                              |
| **connection_name**| Название подключения                                      | varchar(100)       | NOT NULL                                                              |
| **host**           | Хост/адрес сервера                                       | varchar(100)       | NOT NULL                                                              |
| **port**           | Порт сервера                                             | int(10) unsigned   | NOT NULL                                                              |
| **database_name**  | Имя базы данных                                          | varchar(100)       | NOT NULL                                                              |
| **db_username**    | Имя пользователя для подключения к БД                     | varchar(50)        | DEFAULT NULL                                                          |
| **db_password**    | Пароль пользователя для подключения к БД                  | varchar(255)       | DEFAULT NULL                                                          |
| **created_at**     | Время создания записи                                     | timestamp          | DEFAULT current_timestamp()                                           |

---

## **error_logs**

| Название        | Описание                                                | Тип данных         | Ограничения                                                                |
|-----------------|---------------------------------------------------------|--------------------|----------------------------------------------------------------------------|
| **error_id**    | Идентификатор ошибки (**PK**)                           | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                   |
| **user_id**     | Ссылка на пользователя (**FK** → `users.user_id`)       | int(10) unsigned   | DEFAULT NULL                                                               |
| **action_id**   | Ссылка на действие (**FK** → `user_actions.action_id`)  | int(10) unsigned   | DEFAULT NULL                                                               |
| **error_message** | Сообщение об ошибке                                   | text               | DEFAULT NULL                                                               |
| **error_time**  | Время возникновения ошибки                              | timestamp          | DEFAULT current_timestamp()                                                |
| **severity**    | Уровень серьезности ошибки (low, medium, high)         | enum(...)          | DEFAULT 'low'                                                              |

---

## **roles**

| Название       | Описание                                    | Тип данных      | Ограничения                         |
|----------------|---------------------------------------------|-----------------|-------------------------------------|
| **role_id**    | Идентификатор роли (**PK**)                 | int(10) unsigned| NOT NULL, AUTO_INCREMENT            |
| **role_name**  | Название роли (**UK**)                      | varchar(50)     | NOT NULL, UNIQUE                    |
| **description**| Описание роли                               | varchar(255)    | DEFAULT NULL                         |

*(Предустановленные данные: admin, user)*

---

## **user_actions**

| Название       | Описание                                                    | Тип данных         | Ограничения                                                                             |
|----------------|-------------------------------------------------------------|--------------------|-----------------------------------------------------------------------------------------|
| **action_id**  | Идентификатор действия (**PK**)                             | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                                |
| **user_id**    | Ссылка на пользователя (**FK** → `users.user_id`)           | int(10) unsigned   | NOT NULL                                                                               |
| **login_id**   | Ссылка на запись входа (**FK** → `client_logins.login_id`)  | int(10) unsigned   | DEFAULT NULL                                                                           |
| **connection_id** | Ссылка на подключение к БД (**FK** → `database_connections.connection_id`) | int(10) unsigned | DEFAULT NULL                                                                           |
| **action_type**| Тип действия (query, setting_change, background_event, exit, other) | enum(...)  | NOT NULL                                                                               |
| **description**| Описание действия                                          | text               | DEFAULT NULL                                                                           |
| **action_time**| Время выполнения действия                                   | timestamp          | DEFAULT current_timestamp()                                                            |

---

## **user_devices**

| Название       | Описание                                                    | Тип данных         | Ограничения                                              |
|----------------|-------------------------------------------------------------|--------------------|----------------------------------------------------------|
| **device_id**  | Идентификатор устройства (**PK**)                           | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                 |
| **user_id**    | Ссылка на пользователя (**FK** → `users.user_id`)           | int(10) unsigned   | NOT NULL                                                 |
| **device_uuid**| Уникальный идентификатор устройства                         | char(36)           | NOT NULL                                                 |
| **device_name**| Название (описание) устройства                              | varchar(100)       | DEFAULT NULL                                             |
| **ip_address** | IP-адрес устройства                                        | varchar(45)        | DEFAULT NULL                                             |
| **registered_at** | Время регистрации устройства                             | timestamp          | DEFAULT current_timestamp()                              |

---

## **user_reviews**

| Название       | Описание                                                     | Тип данных         | Ограничения                                                                    |
|----------------|--------------------------------------------------------------|--------------------|--------------------------------------------------------------------------------|
| **review_id**  | Идентификатор отзыва (**PK**)                                | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                       |
| **user_id**    | Ссылка на пользователя (**FK** → `users.user_id`)            | int(10) unsigned   | NOT NULL                                                                       |
| **review_text**| Текст отзыва                                                | text               | NOT NULL                                                                       |
| **review_date**| Дата/время создания отзыва                                   | datetime           | DEFAULT current_timestamp()                                                    |
| **error_id**   | Ссылка на ошибку (**FK** → `error_logs.error_id`)            | int(10) unsigned   | DEFAULT NULL                                                                   |
| **rating**     | Оценка (1-5)                                                | tinyint(3) unsigned| NOT NULL, CHECK (`rating` BETWEEN 1 AND 5)                                     |

---

## **user_roles**

| Название   | Описание                                                     | Тип данных       | Ограничения                                               |
|------------|--------------------------------------------------------------|------------------|-----------------------------------------------------------|
| **user_id**| Ссылка на пользователя (**PK**, **FK** → `users.user_id`)    | int(10) unsigned | NOT NULL                                                 |
| **role_id**| Ссылка на роль (**PK**, **FK** → `roles.role_id`)            | int(10) unsigned | NOT NULL                                                 |

*(Составной первичный ключ по (user_id, role_id))*  

---

## **user_settings**

| Название         | Описание                                                          | Тип данных         | Ограничения                                                                        |
|------------------|-------------------------------------------------------------------|--------------------|------------------------------------------------------------------------------------|
| **user_setting_id** | Идентификатор настройки пользователя (**PK**)                  | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                                                           |
| **user_id**      | Ссылка на пользователя (**FK** → `users.user_id`)                 | int(10) unsigned   | NOT NULL                                                                           |
| **login_id**     | Ссылка на запись входа (**FK** → `client_logins.login_id`)        | int(10) unsigned   | NOT NULL                                                                           |
| **action_id**    | Ссылка на действие (**FK** → `user_actions.action_id`)            | int(10) unsigned   | DEFAULT NULL                                                                       |
| **setting_name** | Название настройки                                                | varchar(100)       | NOT NULL                                                                           |
| **setting_value**| Значение настройки                                                | varchar(255)       | NOT NULL                                                                           |
| **changed_at**   | Время изменения настройки                                         | timestamp          | DEFAULT current_timestamp()                                                        |

---

## **users**

| Название     | Описание                                         | Тип данных         | Ограничения                                        |
|--------------|--------------------------------------------------|--------------------|----------------------------------------------------|
| **user_id**  | Идентификатор пользователя (**PK**)              | int(10) unsigned   | NOT NULL, AUTO_INCREMENT                           |
| **username** | Имя пользователя (**UK**)                        | varchar(50)        | NOT NULL, UNIQUE                                   |
| **email**    | Email пользователя (**UK**)                      | varchar(100)       | NOT NULL, UNIQUE                                   |
| **password** | Пароль (хэш)                                     | varchar(255)       | NOT NULL                                           |
| **full_name**| Полное имя пользователя                          | varchar(100)       | DEFAULT NULL                                       |
| **created_at**| Дата/время создания записи                      | timestamp          | DEFAULT current_timestamp()                        |

---

