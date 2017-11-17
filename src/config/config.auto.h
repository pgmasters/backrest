/***********************************************************************************************************************************
Command and Option Configuration

Automatically generated by Build.pm -- do not modify directly.
***********************************************************************************************************************************/
#ifndef CONFIG_CONFIG_AUTO_H
#define CONFIG_CONFIG_AUTO_H

/***********************************************************************************************************************************
Command enum
***********************************************************************************************************************************/
typedef enum
{
    cfgCmdArchiveGet,
    cfgCmdArchivePush,
    cfgCmdBackup,
    cfgCmdCheck,
    cfgCmdExpire,
    cfgCmdHelp,
    cfgCmdInfo,
    cfgCmdLocal,
    cfgCmdRemote,
    cfgCmdRestore,
    cfgCmdStanzaCreate,
    cfgCmdStanzaUpgrade,
    cfgCmdStart,
    cfgCmdStop,
    cfgCmdVersion,
} ConfigCommand;

/***********************************************************************************************************************************
Option enum
***********************************************************************************************************************************/
typedef enum
{
    cfgOptArchiveAsync,
    cfgOptArchiveCheck,
    cfgOptArchiveCopy,
    cfgOptArchiveMaxMb,
    cfgOptArchiveQueueMax,
    cfgOptArchiveTimeout,
    cfgOptBackupCmd,
    cfgOptBackupConfig,
    cfgOptBackupHost,
    cfgOptBackupSshPort,
    cfgOptBackupStandby,
    cfgOptBackupUser,
    cfgOptBufferSize,
    cfgOptChecksumPage,
    cfgOptCmdSsh,
    cfgOptCommand,
    cfgOptCompress,
    cfgOptCompressLevel,
    cfgOptCompressLevelNetwork,
    cfgOptConfig,
    cfgOptDbCmd,
    cfgOptDbConfig =                                                28,
    cfgOptDbHost =                                                  36,
    cfgOptDbInclude =                                               44,
    cfgOptDbPath,
    cfgOptDbPort =                                                  53,
    cfgOptDbSocketPath =                                            61,
    cfgOptDbSshPort =                                               69,
    cfgOptDbTimeout =                                               77,
    cfgOptDbUser,
    cfgOptDelta =                                                   86,
    cfgOptForce,
    cfgOptHardlink,
    cfgOptHostId,
    cfgOptLinkAll,
    cfgOptLinkMap,
    cfgOptLockPath,
    cfgOptLogLevelConsole,
    cfgOptLogLevelFile,
    cfgOptLogLevelStderr,
    cfgOptLogPath,
    cfgOptLogTimestamp,
    cfgOptManifestSaveThreshold,
    cfgOptNeutralUmask,
    cfgOptOnline,
    cfgOptOutput,
    cfgOptProcess,
    cfgOptProcessMax,
    cfgOptProtocolTimeout,
    cfgOptRecoveryOption,
    cfgOptRepoCipherPass,
    cfgOptRepoCipherType,
    cfgOptRepoPath,
    cfgOptRepoS3Bucket,
    cfgOptRepoS3CaFile,
    cfgOptRepoS3CaPath,
    cfgOptRepoS3Endpoint,
    cfgOptRepoS3Host,
    cfgOptRepoS3Key,
    cfgOptRepoS3KeySecret,
    cfgOptRepoS3Region,
    cfgOptRepoS3VerifySsl,
    cfgOptRepoType,
    cfgOptResume,
    cfgOptRetentionArchive,
    cfgOptRetentionArchiveType,
    cfgOptRetentionDiff,
    cfgOptRetentionFull,
    cfgOptSet,
    cfgOptSpoolPath,
    cfgOptStanza,
    cfgOptStartFast,
    cfgOptStopAuto,
    cfgOptTablespaceMap,
    cfgOptTablespaceMapAll,
    cfgOptTarget,
    cfgOptTargetAction,
    cfgOptTargetExclusive,
    cfgOptTargetTimeline,
    cfgOptTest,
    cfgOptTestDelay,
    cfgOptTestPoint,
    cfgOptType,
} ConfigOption;

#endif
