# Collect 系统使用手册

Collect 是一个轻量级的嵌入式 Linux 系统资源监控工具。当前版本采用 C++14 和插件架构实现，支持周期性系统指标采集、结构化输出、异常快照、IPC 配置交互，以及基于 `collect.conf` 的插件加载和参数配置。

本文档描述当前代码已经支持的功能、配置方式、运行方式和限制。

---

## 1. 功能总览

### 已支持功能

| 功能 | 说明 |
| :--- | :--- |
| 插件化采集 | 通过 `.so` 插件扩展采集、输出和快照能力 |
| 配置驱动插件加载 | `collect.conf` 中的 `LoadPlugin` 作为插件加载白名单 |
| 插件参数配置 | `<Plugin name>` 块内的配置会传递给对应插件的 `Configure()` |
| 周期性采集 | read 插件按统一采集周期运行，默认 10 秒 |
| 单次采集 | `-F` 模式执行一次事件循环后退出 |
| JSON 输出 | `json_writer` 输出 AI/LLM 友好的结构化 JSON |
| CSV 输出 | `csv` 插件按 metric 写 CSV 文件或 stdout |
| 文本日志输出 | `logfile` 插件将采集数据写为文本日志 |
| 异常快照 | 支持手动快照和 `SIGUSR1` 触发快照 |
| App 日志打包 | 快照时复制 App 已落盘日志并可打包为 `.tar.gz` |
| IPC 接口 | UNIX Domain Socket 支持查询/修改 `user_config.json` |
| 优雅退出 | `SIGINT` / `SIGTERM` 触发退出和资源清理 |

### 当前限制

| 限制 | 说明 |
| :--- | :--- |
| 插件独立采集周期 | `Interval` 当前是全局采集周期，尚未支持 `<Plugin cpu> Interval 5` 这类插件级周期 |
| HTTP/Prometheus | 尚未实现 REST/HTTP 或 Prometheus 输出 |
| 插件热加载 | 运行中热加载/卸载插件尚未实现 |
| protobuf 输出 | 尚未实现 |
| tag 标签系统 | 尚未实现多维标签 |
| App 配置主动推送 | 目前 IPC 是请求-响应模型，Collect 不主动推送配置变更 |
| 交叉编译 toolchain | 尚未提供 ARM toolchain 文件 |

---

## 2. 编译与安装

### 前置条件

- CMake 3.14 或更高版本
- C++14 编译器，如 GCC 或 Clang
- Linux 环境用于生产运行，需支持 `pthread`、`dlopen`、UNIX Domain Socket 和 `/proc`
- macOS 可用于开发构建和部分单元测试，但部分插件依赖 Linux `/proc` 或系统命令

### 编译步骤

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

macOS 没有 `nproc` 时可使用：

```bash
make -j$(sysctl -n hw.ncpu)
```

### 构建产物

默认构建产物位于：

| 路径 | 说明 |
| :--- | :--- |
| `build/bin/collect` | 主程序 |
| `build/bin/modules/<plugin>/<plugin>.so` | 插件动态库 |
| `build/bin/share/` | 拷贝后的默认配置文件 |

### 安装

```bash
sudo make install
```

默认安装内容：

| 内容 | 默认位置 |
| :--- | :--- |
| 可执行文件 | `/usr/local/bin/collect` |
| share 配置文件 | `/usr/local/share` |

插件安装路径建议在部署时显式规划，并通过 `collect.conf` 的 `PluginDir` 或命令行 `-p` 指定。

---

## 3. 运行方式

### daemon 模式

```bash
./build/bin/collect [选项]
```

常用示例：

```bash
./build/bin/collect \
  -c ./share/collect.conf \
  -p ./build/bin/modules \
  -u ./share/user_config.json \
  -s /tmp/collect.sock
```

### 单次采集模式

`-F` 表示执行一次主循环后退出，适合脚本调用或快速验证。

```bash
./build/bin/collect \
  -c ./share/collect.conf \
  -p ./build/bin/modules \
  -u ./share/user_config.json \
  -F
```

### 命令行参数

| 参数 | 说明 | 默认值 |
| :--- | :--- | :--- |
| `-c <file>` | 指定 `collect.conf` 路径 | `/etc/collect/collect.conf` |
| `-p <dir>` | 指定插件目录 | `/usr/lib/collect/modules` |
| `-u <file>` | 指定 `user_config.json` 路径 | `/etc/collect/user_config.json` |
| `-s <socket>` | 指定 IPC socket 路径 | `/tmp/collect.sock` |
| `-i <seconds>` | 指定默认采集周期，单位秒 | `10.0` |
| `-F` | 单次采集模式 | 关闭 |

配置加载顺序是：先解析命令行，再加载 `collect.conf`。如果 `collect.conf` 中显式配置了 `PluginDir` 或 `Interval`，会覆盖命令行中的 `-p` 或 `-i`。部署时建议只在一个地方配置同一项，避免混淆。

### 停止运行

| 信号 | 行为 |
| :--- | :--- |
| `SIGINT` | 优雅退出 |
| `SIGTERM` | 优雅退出 |
| `SIGUSR1` | daemon 模式下生成一次异常快照 |

示例：

```bash
kill -TERM <collect_pid>
kill -USR1 <collect_pid>
```

---

## 4. collect.conf 主配置

`collect.conf` 负责配置插件目录、类型库、采集周期、插件加载列表、插件参数和快照默认路径。

### 全局配置项

| 配置项 | 是否支持 | 说明 |
| :--- | :--- | :--- |
| `PluginDir "<dir>"` | 支持 | 插件根目录，目录下应是 `<name>/<name>.so` |
| `TypesDB "<file>"` | 支持加载 | 加载 types.db；当前内置插件主要自行构造 `DataSet` |
| `Interval <seconds>` | 支持 | 全局 read 插件采集周期 |
| `SnapshotDir "<dir>"` | 支持 | 快照输出根目录 |
| `AppLogDir "<dir>"` | 支持 | 快照时复制 App 日志的来源目录 |
| `SnapshotPack true/false` | 支持 | 是否生成 `.tar.gz` 快照包 |
| 其他全局项 | 部分保留 | 会被解析为 generic global，但不一定参与运行行为 |

示例：

```text
PluginDir "/mnt/data/collect/modules"
TypesDB "/mnt/data/collect/share/types.db"
Interval 10

SnapshotDir "/mnt/data/collect/snapshots"
AppLogDir "/mnt/data/app/logs"
SnapshotPack true
```

### 插件加载

`LoadPlugin` 是加载白名单。只要配置文件中出现至少一个 `LoadPlugin`，Collect 就只加载这些插件。

```text
LoadPlugin cpu
LoadPlugin memory
LoadPlugin df
LoadPlugin uptime
LoadPlugin json_writer
```

如果配置文件中没有任何 `LoadPlugin`，Collect 会兼容旧行为：扫描 `PluginDir` 下所有可用插件并全部加载。

插件目录结构必须类似：

```text
modules/
  cpu/cpu.so
  memory/memory.so
  json_writer/json_writer.so
```

### 插件参数派发

`<Plugin name>` 块中的每个配置项都会传递给对应插件：

```text
<Plugin json_writer>
  OutputFile "/mnt/data/collect/metrics.json"
  StdOut false
  Host "smart-camera-01"
</Plugin>
```

等价于运行时调用：

```cpp
json_writer->Configure("OutputFile", "/mnt/data/collect/metrics.json");
json_writer->Configure("StdOut", "false");
json_writer->Configure("Host", "smart-camera-01");
```

未被插件识别的配置项会被忽略，不会导致启动失败。

---

## 5. 插件能力与配置

### 插件分类

| 类型 | 说明 |
| :--- | :--- |
| Read 插件 | 周期性采集指标，通过 `Dispatch()` 投递到写队列 |
| Write 插件 | 消费采集数据并输出到文件、stdout 或日志 |
| Flush 插件 | 支持 `Flush()`，用于刷新缓冲或导出日志 |
| Snapshot 插件 | 异常快照时输出一次诊断文件 |

一个插件可以同时支持多种能力，例如 `json_writer` 同时支持 write 和 flush。

### Read 插件

| 插件 | 数据来源 | 输出内容 | 支持配置 |
| :--- | :--- | :--- | :--- |
| `cpu` | `/proc/stat` | CPU 使用率、CPU 数量 | `ReportByCpu`、`ReportByState`、`ValuesPercentage` |
| `memory` | `/proc/meminfo` | 内存 used/free/buffered/cached/slab/available | `ValuesAbsolute`、`ValuesPercentage` |
| `df` | `statvfs()` | 挂载点 free/reserved/used 或百分比 | `MountPoint`、`ValuesAbsolute`、`ValuesPercentage` |
| `uptime` | `sysinfo()` 或 macOS boot time | 系统启动时长，单位秒 | 无 |

#### cpu

```text
LoadPlugin cpu

<Plugin cpu>
  ReportByCpu true
  ReportByState true
  ValuesPercentage true
</Plugin>
```

配置说明：

| 配置项 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `ReportByCpu` | `true` | 按单个 CPU 核输出 |
| `ReportByState` | `true` | 输出 user/system/idle/wait 等状态 |
| `ValuesPercentage` | `true` | 接收该配置；当前 CPU 插件输出为百分比指标 |

#### memory

```text
LoadPlugin memory

<Plugin memory>
  ValuesAbsolute true
  ValuesPercentage false
</Plugin>
```

| 配置项 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `ValuesAbsolute` | `true` | 输出字节数 |
| `ValuesPercentage` | `false` | 输出百分比 |

#### df

```text
LoadPlugin df

<Plugin df>
  MountPoint "/"
  MountPoint "/mnt/data"
  ValuesAbsolute true
  ValuesPercentage false
</Plugin>
```

| 配置项 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `MountPoint` | `/` | 可配置多次，指定要采集的挂载点 |
| `ValuesAbsolute` | `true` | 输出字节数 |
| `ValuesPercentage` | `false` | 输出百分比 |

当前 `df` 插件不处理 `Device`、`FSType`、`IgnoreSelected` 等 collectd 兼容字段。

#### uptime

```text
LoadPlugin uptime
```

`uptime` 当前无插件级配置项。

### Write 插件

| 插件 | 输出目标 | 支持配置 |
| :--- | :--- | :--- |
| `json_writer` | stdout 或 JSON 文件 | `OutputFile`、`StdOut`、`Host` |
| `csv` | stdout 或 CSV 文件 | `DataDir`、`FileDate` |
| `logfile` | 文本日志文件 | `BaseDir`、`LogFile` |

#### json_writer

```text
LoadPlugin json_writer

<Plugin json_writer>
  OutputFile "/mnt/data/collect/metrics.json"
  StdOut false
  Host "smart-camera-01"
</Plugin>
```

| 配置项 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `OutputFile` | 空 | 追加写入 JSON lines 文件 |
| `StdOut` | `true` | 是否输出到 stdout |
| `Host` | 空 | 写入 JSON 的 `host` 字段 |

如果同时配置了 `OutputFile` 和 `StdOut true`，数据会同时写文件和 stdout。

单条 JSON 示例：

```json
{"timestamp":"2026-05-11T00:00:00+08:00","host":"smart-camera-01","plugin":"memory","type":"memory","type_instance":"used","value":123.0,"data_type":"gauge","ds_name":"value","ds_type":"memory"}
```

#### csv

```text
LoadPlugin csv

<Plugin csv>
  DataDir "/mnt/data/collect/csv"
  FileDate false
</Plugin>
```

| 配置项 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `DataDir` | 空 | CSV 输出根目录；值为 `stdout` 时输出 PUTVAL 行 |
| `FileDate` | `false` | 文件名追加 `-YYYY-MM-DD` |

CSV 插件按 `plugin[-instance]/type[-instance]` 生成文件名。例如 `cpu-0/percent-user`。因此使用文件输出时，需要确保相关父目录存在，或者在部署脚本中提前创建。

CSV 文件内容示例：

```csv
epoch,value
12345.678,42.000
```

stdout 模式示例：

```text
<Plugin csv>
  DataDir "stdout"
</Plugin>
```

输出格式：

```text
PUTVAL cpu-0/percent-user 12345.678,42.000
```

#### logfile

```text
LoadPlugin logfile

<Plugin logfile>
  BaseDir "/mnt/data/collect/log"
  LogFile "collect_data.log"
</Plugin>
```

| 配置项 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `BaseDir` | 空 | 日志目录，必须配置 |
| `LogFile` | `collect_data.log` | 日志文件名 |

`logfile` 的 `Flush()` 会执行：

```text
/mnt/app/toolbox log_record export <BaseDir>
```

部署环境需要保证该工具存在，否则 flush 会失败。

### Snapshot 插件

| 插件 | 输出文件 | 支持配置 |
| :--- | :--- | :--- |
| `network` | `network.txt` | `BaseDir` |
| `dmesg` | 默认 `dmesg.txt` | `BaseDir`、`OutputFile` |
| `thread` | `thread.txt` | `BaseDir`、`TargetProcess` |

daemon 或 snapshot 命令创建快照时，会把本次快照目录传给 snapshot 插件。此时插件优先写入本次快照目录，`BaseDir` 主要用于直接调用插件 snapshot 时的兜底目录。

#### network

```text
LoadPlugin network

<Plugin network>
  BaseDir "/mnt/data/collect/snapshots"
</Plugin>
```

采集内容包括：

- `/proc/net/dev`
- `ip addr show`
- `ip route show`
- `/etc/resolv.conf`
- `netstat -anp`
- `arp -n`

#### dmesg

```text
LoadPlugin dmesg

<Plugin dmesg>
  OutputFile "dmesg.txt"
</Plugin>
```

通过 `dmesg` 命令导出内核 ring buffer。

#### thread

```text
LoadPlugin thread

<Plugin thread>
  TargetProcess "m320_app"
</Plugin>
```

| 配置项 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `TargetProcess` | `m320_app` | 未通过 `--pid` 指定时，按进程名查找目标 PID |
| `BaseDir` | 空 | 直接调用 snapshot 时的兜底输出目录 |

---

## 6. 异常快照

### 手动快照

```bash
./build/bin/collect snapshot \
  --reason manual \
  --app-log-dir /mnt/data/app/logs \
  --out /mnt/data/collect/snapshots \
  -p ./build/bin/modules
```

### 参数说明

| 参数 | 说明 |
| :--- | :--- |
| `--reason <text>` | 快照原因，如 `manual`、`app_crash` |
| `--app-log-dir <dir>` | App 已落盘日志目录，Collect 会复制到 `app_logs/` |
| `--out <dir>` | 快照输出根目录 |
| `--pid <pid>` | thread 插件使用的目标进程 PID |
| `--no-pack` | 只保留快照目录，不生成 `.tar.gz` |
| `-c/--config <file>` | 指定 `collect.conf` |
| `-p/--plugin-dir <dir>` | 指定插件目录 |
| `-u/--user-config <file>` | 指定 `user_config.json` |

### daemon 中触发快照

```bash
kill -USR1 <collect_pid>
```

### 快照输出结构

```text
snapshot_YYYYMMDD_HHMMSS/
  summary.json
  app_logs/
  dmesg.txt
  network.txt
  thread.txt
snapshot_YYYYMMDD_HHMMSS.tar.gz
```

`summary.json` 会记录快照原因、输出目录、App 日志复制状态、插件执行失败数和打包状态。

---

## 7. user_config.json 与 IPC

`user_config.json` 面向 App 交互配置，默认路径是 `/etc/collect/user_config.json`，可通过 `-u` 指定。

示例：

```json
{
  "modules": {
    "app": {
      "log_level": "INFO",
      "fifo_cache": true
    },
    "operator": {
      "log_level": "WARNING",
      "fifo_cache": false
    },
    "dsp": {
      "log_level": "ERROR",
      "fifo_cache": true
    }
  },
  "user_log": {
    "enabled": true,
    "format": "csv",
    "fields": ["timestamp", "username", "ip", "action"]
  },
  "output_log": {
    "enabled": true,
    "format": "txt",
    "fields": ["timestamp", "content"]
  },
  "system": {
    "log_redirect": false,
    "debug_mode": false,
    "serial_control": true,
    "watchdog": true
  }
}
```

### IPC 基础

Collect 通过 UNIX Domain Socket 提供请求-响应式 IPC 服务。

启动时指定 socket：

```bash
./build/bin/collect -s /tmp/collect.sock
```

请求是 JSON 对象，响应也是 JSON 对象：

```json
{
  "code": 0,
  "message": "ok",
  "data": {}
}
```

### 支持的 IPC 命令

| 命令 | 请求字段 | 说明 |
| :--- | :--- | :--- |
| `get_config` | 无 | 返回当前 `user_config.json` |
| `set_log_level` | `module`、`level` | 设置模块日志级别并保存 |
| `set_debug_mode` | `enabled` | 设置 `system.debug_mode` 并保存 |
| `reload_config` | 无 | 从原路径重新加载 `user_config.json` |
| `get_status` | 无 | 返回运行状态和插件数量 |

### IPC 示例

查询配置：

```bash
printf '{"cmd":"get_config"}' | socat - UNIX-CONNECT:/tmp/collect.sock
```

设置日志级别：

```bash
printf '{"cmd":"set_log_level","module":"app","level":"DEBUG"}' \
  | socat - UNIX-CONNECT:/tmp/collect.sock
```

开启 debug mode：

```bash
printf '{"cmd":"set_debug_mode","enabled":true}' \
  | socat - UNIX-CONNECT:/tmp/collect.sock
```

查询状态：

```bash
printf '{"cmd":"get_status"}' | socat - UNIX-CONNECT:/tmp/collect.sock
```

---

## 8. 数据输出与流转

### 数据流

```text
Read 插件
  -> Dispatch(DataSet, ValueList)
  -> WriteQueue
  -> 所有 Write 插件的 Write(DataSet, ValueList)
```

### DataSet

`DataSet` 描述 metric 的数据源：

```cpp
DataSet {
  type: "memory",
  sources: [
    { name: "value", type: Gauge, min: 0, max: 0 }
  ]
}
```

### ValueList

`ValueList` 承载一次采集结果：

```cpp
ValueList {
  plugin: "memory",
  pluginInstance: "",
  type: "memory",
  typeInstance: "used",
  values: [ Gauge(123.0) ],
  time: CdTime,
  interval: CdTime
}
```

### JSON 输出字段

`json_writer` 输出字段包括：

| 字段 | 说明 |
| :--- | :--- |
| `timestamp` | ISO 8601 时间 |
| `host` | 可选，由 `Host` 配置 |
| `plugin` | 插件名 |
| `plugin_instance` | 插件实例，可选 |
| `type` | metric 类型 |
| `type_instance` | metric 实例，可选 |
| `interval_sec` | 采集周期，可选 |
| `value` | 单值 metric 的值 |
| `values` | 多值 metric 的数组 |
| `data_type` | `gauge`、`counter`、`derive`、`absolute` |
| `ds_name` | 数据源名称 |
| `ds_type` | DataSet 类型 |

---

## 9. 日志与排障

### 查看运行日志

Collect 默认将运行日志输出到 stderr。前台运行时可直接查看；后台运行时建议重定向：

```bash
./build/bin/collect -c ./share/collect.conf 2> /tmp/collect.log
```

### 常见问题

**Q: 启动时提示 `No plugins found`？**
A: 检查 `PluginDir` 或 `-p` 是否指向插件根目录。正确目录应包含 `cpu/cpu.so`、`memory/memory.so` 这类子目录。

**Q: 配置了 `LoadPlugin json_writer` 但没有 JSON 输出？**
A: 检查 `<Plugin json_writer>` 中是否设置了 `StdOut false` 且没有设置 `OutputFile`。默认 `StdOut` 是 `true`，如果关闭 stdout，应配置 `OutputFile`。

**Q: `<Plugin>` 中某些配置没有效果？**
A: 只有插件代码识别的配置项才会生效。为了兼容 collectd 风格配置，未知字段会被忽略。

**Q: CSV 文件没有创建？**
A: 检查 `DataDir` 是否存在，以及 metric 文件路径中的父目录是否已经创建。当前 CSV 插件不会自动创建多级父目录。

**Q: snapshot 中缺少 `thread.txt`？**
A: 检查 `--pid` 是否正确，或 `TargetProcess` 对应进程是否存在。

**Q: macOS 上插件读取失败？**
A: 部分 read/snapshot 插件依赖 Linux `/proc`、`dmesg`、`ip`、`netstat` 等环境。macOS 主要用于开发构建和单元测试，生产运行建议使用目标 Linux 环境。

---

## 10. 推荐配置示例

### JSON 输出到文件

```text
PluginDir "/mnt/data/collect/modules"
Interval 10

LoadPlugin cpu
LoadPlugin memory
LoadPlugin df
LoadPlugin uptime
LoadPlugin json_writer

<Plugin df>
  MountPoint "/"
  MountPoint "/mnt/data"
  ValuesAbsolute true
  ValuesPercentage false
</Plugin>

<Plugin json_writer>
  OutputFile "/mnt/data/collect/metrics.json"
  StdOut false
  Host "smart-camera-01"
</Plugin>
```

### 异常快照配置

```text
PluginDir "/mnt/data/collect/modules"

SnapshotDir "/mnt/data/collect/snapshots"
AppLogDir "/mnt/data/app/logs"
SnapshotPack true

LoadPlugin network
LoadPlugin dmesg
LoadPlugin thread

<Plugin thread>
  TargetProcess "m320_app"
</Plugin>
```

手动触发：

```bash
./build/bin/collect snapshot \
  -c ./share/collect.conf \
  -p ./build/bin/modules \
  --reason manual \
  --out /mnt/data/collect/snapshots
```
