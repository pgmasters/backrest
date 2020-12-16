/***********************************************************************************************************************************
Command and Option Configuration

Automatically generated by Build.pm -- do not modify directly.
***********************************************************************************************************************************/

/***********************************************************************************************************************************
Command constants
***********************************************************************************************************************************/
STRING_EXTERN(CFGCMD_ARCHIVE_GET_STR,                               CFGCMD_ARCHIVE_GET);
STRING_EXTERN(CFGCMD_ARCHIVE_PUSH_STR,                              CFGCMD_ARCHIVE_PUSH);
STRING_EXTERN(CFGCMD_BACKUP_STR,                                    CFGCMD_BACKUP);
STRING_EXTERN(CFGCMD_CHECK_STR,                                     CFGCMD_CHECK);
STRING_EXTERN(CFGCMD_EXPIRE_STR,                                    CFGCMD_EXPIRE);
STRING_EXTERN(CFGCMD_HELP_STR,                                      CFGCMD_HELP);
STRING_EXTERN(CFGCMD_INFO_STR,                                      CFGCMD_INFO);
STRING_EXTERN(CFGCMD_REPO_CREATE_STR,                               CFGCMD_REPO_CREATE);
STRING_EXTERN(CFGCMD_REPO_GET_STR,                                  CFGCMD_REPO_GET);
STRING_EXTERN(CFGCMD_REPO_LS_STR,                                   CFGCMD_REPO_LS);
STRING_EXTERN(CFGCMD_REPO_PUT_STR,                                  CFGCMD_REPO_PUT);
STRING_EXTERN(CFGCMD_REPO_RM_STR,                                   CFGCMD_REPO_RM);
STRING_EXTERN(CFGCMD_RESTORE_STR,                                   CFGCMD_RESTORE);
STRING_EXTERN(CFGCMD_STANZA_CREATE_STR,                             CFGCMD_STANZA_CREATE);
STRING_EXTERN(CFGCMD_STANZA_DELETE_STR,                             CFGCMD_STANZA_DELETE);
STRING_EXTERN(CFGCMD_STANZA_UPGRADE_STR,                            CFGCMD_STANZA_UPGRADE);
STRING_EXTERN(CFGCMD_START_STR,                                     CFGCMD_START);
STRING_EXTERN(CFGCMD_STOP_STR,                                      CFGCMD_STOP);
STRING_EXTERN(CFGCMD_VERIFY_STR,                                    CFGCMD_VERIFY);
STRING_EXTERN(CFGCMD_VERSION_STR,                                   CFGCMD_VERSION);

/***********************************************************************************************************************************
Command data
***********************************************************************************************************************************/
static ConfigCommandData configCommandData[CFG_COMMAND_TOTAL] = CONFIG_COMMAND_LIST
(
    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_ARCHIVE_GET)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeArchive)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_ARCHIVE_PUSH)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(true)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeArchive)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_BACKUP)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(true)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(true)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeBackup)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_CHECK)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_EXPIRE)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(true)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeBackup)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_HELP)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelDebug)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_INFO)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelDebug)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_REPO_CREATE)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_REPO_GET)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelDebug)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_REPO_LS)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelDebug)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_REPO_PUT)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelDebug)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_REPO_RM)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelDebug)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_RESTORE)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_STANZA_CREATE)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(true)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeAll)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_STANZA_DELETE)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(true)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeAll)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_STANZA_UPGRADE)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(true)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeAll)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_START)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_STOP)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_VERIFY)

        CONFIG_COMMAND_LOG_FILE(true)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelInfo)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )

    CONFIG_COMMAND
    (
        CONFIG_COMMAND_NAME(CFGCMD_VERSION)

        CONFIG_COMMAND_LOG_FILE(false)
        CONFIG_COMMAND_LOG_LEVEL_DEFAULT(logLevelDebug)
        CONFIG_COMMAND_LOCK_REQUIRED(false)
        CONFIG_COMMAND_LOCK_REMOTE_REQUIRED(false)
        CONFIG_COMMAND_LOCK_TYPE(lockTypeNone)
    )
)

/***********************************************************************************************************************************
Option constants
***********************************************************************************************************************************/
STRING_EXTERN(CFGOPT_ARCHIVE_ASYNC_STR,                             CFGOPT_ARCHIVE_ASYNC);
STRING_EXTERN(CFGOPT_ARCHIVE_CHECK_STR,                             CFGOPT_ARCHIVE_CHECK);
STRING_EXTERN(CFGOPT_ARCHIVE_COPY_STR,                              CFGOPT_ARCHIVE_COPY);
STRING_EXTERN(CFGOPT_ARCHIVE_GET_QUEUE_MAX_STR,                     CFGOPT_ARCHIVE_GET_QUEUE_MAX);
STRING_EXTERN(CFGOPT_ARCHIVE_MODE_STR,                              CFGOPT_ARCHIVE_MODE);
STRING_EXTERN(CFGOPT_ARCHIVE_PUSH_QUEUE_MAX_STR,                    CFGOPT_ARCHIVE_PUSH_QUEUE_MAX);
STRING_EXTERN(CFGOPT_ARCHIVE_TIMEOUT_STR,                           CFGOPT_ARCHIVE_TIMEOUT);
STRING_EXTERN(CFGOPT_BACKUP_STANDBY_STR,                            CFGOPT_BACKUP_STANDBY);
STRING_EXTERN(CFGOPT_BUFFER_SIZE_STR,                               CFGOPT_BUFFER_SIZE);
STRING_EXTERN(CFGOPT_CHECKSUM_PAGE_STR,                             CFGOPT_CHECKSUM_PAGE);
STRING_EXTERN(CFGOPT_CIPHER_PASS_STR,                               CFGOPT_CIPHER_PASS);
STRING_EXTERN(CFGOPT_CMD_SSH_STR,                                   CFGOPT_CMD_SSH);
STRING_EXTERN(CFGOPT_COMPRESS_STR,                                  CFGOPT_COMPRESS);
STRING_EXTERN(CFGOPT_COMPRESS_LEVEL_STR,                            CFGOPT_COMPRESS_LEVEL);
STRING_EXTERN(CFGOPT_COMPRESS_LEVEL_NETWORK_STR,                    CFGOPT_COMPRESS_LEVEL_NETWORK);
STRING_EXTERN(CFGOPT_COMPRESS_TYPE_STR,                             CFGOPT_COMPRESS_TYPE);
STRING_EXTERN(CFGOPT_CONFIG_STR,                                    CFGOPT_CONFIG);
STRING_EXTERN(CFGOPT_CONFIG_INCLUDE_PATH_STR,                       CFGOPT_CONFIG_INCLUDE_PATH);
STRING_EXTERN(CFGOPT_CONFIG_PATH_STR,                               CFGOPT_CONFIG_PATH);
STRING_EXTERN(CFGOPT_DB_INCLUDE_STR,                                CFGOPT_DB_INCLUDE);
STRING_EXTERN(CFGOPT_DB_TIMEOUT_STR,                                CFGOPT_DB_TIMEOUT);
STRING_EXTERN(CFGOPT_DELTA_STR,                                     CFGOPT_DELTA);
STRING_EXTERN(CFGOPT_DRY_RUN_STR,                                   CFGOPT_DRY_RUN);
STRING_EXTERN(CFGOPT_EXCLUDE_STR,                                   CFGOPT_EXCLUDE);
STRING_EXTERN(CFGOPT_EXEC_ID_STR,                                   CFGOPT_EXEC_ID);
STRING_EXTERN(CFGOPT_EXPIRE_AUTO_STR,                               CFGOPT_EXPIRE_AUTO);
STRING_EXTERN(CFGOPT_FILTER_STR,                                    CFGOPT_FILTER);
STRING_EXTERN(CFGOPT_FORCE_STR,                                     CFGOPT_FORCE);
STRING_EXTERN(CFGOPT_IGNORE_MISSING_STR,                            CFGOPT_IGNORE_MISSING);
STRING_EXTERN(CFGOPT_IO_TIMEOUT_STR,                                CFGOPT_IO_TIMEOUT);
STRING_EXTERN(CFGOPT_LINK_ALL_STR,                                  CFGOPT_LINK_ALL);
STRING_EXTERN(CFGOPT_LINK_MAP_STR,                                  CFGOPT_LINK_MAP);
STRING_EXTERN(CFGOPT_LOCK_PATH_STR,                                 CFGOPT_LOCK_PATH);
STRING_EXTERN(CFGOPT_LOG_LEVEL_CONSOLE_STR,                         CFGOPT_LOG_LEVEL_CONSOLE);
STRING_EXTERN(CFGOPT_LOG_LEVEL_FILE_STR,                            CFGOPT_LOG_LEVEL_FILE);
STRING_EXTERN(CFGOPT_LOG_LEVEL_STDERR_STR,                          CFGOPT_LOG_LEVEL_STDERR);
STRING_EXTERN(CFGOPT_LOG_PATH_STR,                                  CFGOPT_LOG_PATH);
STRING_EXTERN(CFGOPT_LOG_SUBPROCESS_STR,                            CFGOPT_LOG_SUBPROCESS);
STRING_EXTERN(CFGOPT_LOG_TIMESTAMP_STR,                             CFGOPT_LOG_TIMESTAMP);
STRING_EXTERN(CFGOPT_MANIFEST_SAVE_THRESHOLD_STR,                   CFGOPT_MANIFEST_SAVE_THRESHOLD);
STRING_EXTERN(CFGOPT_NEUTRAL_UMASK_STR,                             CFGOPT_NEUTRAL_UMASK);
STRING_EXTERN(CFGOPT_ONLINE_STR,                                    CFGOPT_ONLINE);
STRING_EXTERN(CFGOPT_OUTPUT_STR,                                    CFGOPT_OUTPUT);
STRING_EXTERN(CFGOPT_PG_STR,                                        CFGOPT_PG);
STRING_EXTERN(CFGOPT_PROCESS_STR,                                   CFGOPT_PROCESS);
STRING_EXTERN(CFGOPT_PROCESS_MAX_STR,                               CFGOPT_PROCESS_MAX);
STRING_EXTERN(CFGOPT_PROTOCOL_TIMEOUT_STR,                          CFGOPT_PROTOCOL_TIMEOUT);
STRING_EXTERN(CFGOPT_RAW_STR,                                       CFGOPT_RAW);
STRING_EXTERN(CFGOPT_RECOVERY_OPTION_STR,                           CFGOPT_RECOVERY_OPTION);
STRING_EXTERN(CFGOPT_RECURSE_STR,                                   CFGOPT_RECURSE);
STRING_EXTERN(CFGOPT_REMOTE_TYPE_STR,                               CFGOPT_REMOTE_TYPE);
STRING_EXTERN(CFGOPT_REPO_STR,                                      CFGOPT_REPO);
STRING_EXTERN(CFGOPT_RESUME_STR,                                    CFGOPT_RESUME);
STRING_EXTERN(CFGOPT_SCK_BLOCK_STR,                                 CFGOPT_SCK_BLOCK);
STRING_EXTERN(CFGOPT_SCK_KEEP_ALIVE_STR,                            CFGOPT_SCK_KEEP_ALIVE);
STRING_EXTERN(CFGOPT_SET_STR,                                       CFGOPT_SET);
STRING_EXTERN(CFGOPT_SORT_STR,                                      CFGOPT_SORT);
STRING_EXTERN(CFGOPT_SPOOL_PATH_STR,                                CFGOPT_SPOOL_PATH);
STRING_EXTERN(CFGOPT_STANZA_STR,                                    CFGOPT_STANZA);
STRING_EXTERN(CFGOPT_START_FAST_STR,                                CFGOPT_START_FAST);
STRING_EXTERN(CFGOPT_STOP_AUTO_STR,                                 CFGOPT_STOP_AUTO);
STRING_EXTERN(CFGOPT_TABLESPACE_MAP_STR,                            CFGOPT_TABLESPACE_MAP);
STRING_EXTERN(CFGOPT_TABLESPACE_MAP_ALL_STR,                        CFGOPT_TABLESPACE_MAP_ALL);
STRING_EXTERN(CFGOPT_TARGET_STR,                                    CFGOPT_TARGET);
STRING_EXTERN(CFGOPT_TARGET_ACTION_STR,                             CFGOPT_TARGET_ACTION);
STRING_EXTERN(CFGOPT_TARGET_EXCLUSIVE_STR,                          CFGOPT_TARGET_EXCLUSIVE);
STRING_EXTERN(CFGOPT_TARGET_TIMELINE_STR,                           CFGOPT_TARGET_TIMELINE);
STRING_EXTERN(CFGOPT_TCP_KEEP_ALIVE_COUNT_STR,                      CFGOPT_TCP_KEEP_ALIVE_COUNT);
STRING_EXTERN(CFGOPT_TCP_KEEP_ALIVE_IDLE_STR,                       CFGOPT_TCP_KEEP_ALIVE_IDLE);
STRING_EXTERN(CFGOPT_TCP_KEEP_ALIVE_INTERVAL_STR,                   CFGOPT_TCP_KEEP_ALIVE_INTERVAL);
STRING_EXTERN(CFGOPT_TYPE_STR,                                      CFGOPT_TYPE);
