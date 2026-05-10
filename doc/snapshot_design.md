# Collect Snapshot Design

## Goal

Collect supports one-shot diagnostic snapshots for embedded Linux devices. A
snapshot is intended to run after an App anomaly, crash preparation path, manual
debug command, or `SIGUSR1` trigger. Collect does not assume the App is still
alive after a crash; it packages App logs that already exist on flash.

## Snapshot Flow

```text
trigger: CLI / SIGUSR1
  -> create snapshot_YYYYMMDD_HHMMSS directory
  -> copy App log directory if configured
  -> run snapshot plugins
  -> write summary.json
  -> optionally create snapshot_YYYYMMDD_HHMMSS.tar.gz
```

The directory name is the operational identifier. There is no separate snapshot
ID in the first version.

## Directory Layout

```text
snapshot_YYYYMMDD_HHMMSS/
  summary.json
  app_logs/
  dmesg.txt
  network.txt
  thread.txt
snapshot_YYYYMMDD_HHMMSS.tar.gz
```

`summary.json` records reason, output paths, App log copy status, target PID,
plugin failure count, and final result code.

## Plugin Capabilities

Plugins now have a fourth capability:

```text
HasRead      periodic metric collection
HasWrite     periodic metric output
HasFlush     flush buffered writer data
HasSnapshot  write one-shot diagnostic files under snapshotDir
```

`network`, `thread`, and `dmesg` are snapshot plugins. They are not periodic
read plugins in this design.

## Triggers

Manual snapshot:

```bash
collect snapshot --reason manual \
  --app-log-dir /mnt/data/app/logs \
  --out /mnt/data/collect/snapshots
```

Daemon snapshot:

```bash
kill -USR1 <collect_pid>
```

The signal handler only sets a pending flag. The main loop performs the actual
filesystem and plugin work.

## App Log Contract

Collect does not parse App business logs. The App owns its ring buffer and log
format. Collect copies a configured App log directory into `app_logs/` and then
packages it with the system snapshot. If the App already crashed and the
directory is missing, the snapshot still succeeds and records
`"app_logs":"missing"`.

