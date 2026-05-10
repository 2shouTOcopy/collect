# Collect 系统使用手册

Collect 是一个轻量级的嵌入式 Linux 系统资源监控工具。它通过插件架构收集 CPU、内存、网络等系统指标，并支持通过 JSON、CSV 或日志文件输出，同时支持 IPC 接口供外部应用交互。

## 1. 编译与安装

### 前置条件
- CMake 3.14 或更高版本
- C++14 编译器 (如 GCC 5+ 或 Clang 3.8+)
- Linux 环境 (支持 pthread, dl)

### 编译步骤

```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 生成构建文件
cmake ..

# 3. 编译
make -j$(nproc)
```

编译完成后，相关文件将生成在 `build/bin` 和 `build/lib` 目录下：
- `build/bin/collect`: 主程序守护进程
- `build/lib/collect/modules/*.so`: 各种插件库

### 安装 (可选)

```bash
sudo make install
```
默认安装路径：
- 可执行文件: `/usr/local/bin/collect`
- 插件目录: `/usr/local/lib/collect/modules` (具体路径视 cmake 配置而定，通常建议手动指定或通过 `-p` 运行)

---

## 2. 运行指南

Collect 以守护进程方式运行，支持多种命令行参数进行配置。

### 启动命令

```bash
./bin/collect [选项]
```

### 命令行参数

| 参数           | 说明                             | 默认值                          |
| :------------- | :------------------------------- | :------------------------------ |
| `-c <file>`    | 指定主配置文件路径               | `/etc/collect/collect.conf`     |
| `-p <dir>`     | 指定插件目录路径                 | `/usr/lib/collect/modules`      |
| `-u <file>`    | 指定用户配置文件路径 (JSON)      | `/etc/collect/user_config.json` |
| `-s <socket>`  | 指定 IPC 通信 Socket 路径        | `/tmp/collect.sock`             |
| `-i <seconds>` | 指定默认采集间隔 (秒)            | `10.0`                          |
| `-F`           | 单次采集模式，采集一次后立即退出 | -                               |

### 运行示例

假设插件位于 `build/lib/collect/modules`，以 5 秒间隔运行：

```bash
./bin/collect -p ../build/lib/collect/modules -i 5
```

### 单次采集模式 (-F)

使用 `-F` 参数可以仅采集一次系统数据后立即退出，适用于脚本调用或一次性诊断场景：

```bash
./bin/collect -p ../build/lib/collect/modules -F
```

### 异常快照模式

手动生成一次诊断快照：

```bash
./bin/collect snapshot \
  --reason manual \
  --app-log-dir /mnt/data/app/logs \
  --out /mnt/data/collect/snapshots
```

常用参数：

| 参数 | 说明 |
| :--- | :--- |
| `--reason <text>` | 触发原因，如 `manual`、`app_crash` |
| `--app-log-dir <dir>` | App 已经落盘的日志目录，Collect 只复制和打包 |
| `--out <dir>` | 快照输出根目录 |
| `--pid <pid>` | thread snapshot 使用的目标进程 PID |
| `--no-pack` | 只保留快照目录，不生成 `.tar.gz` |
| `-c/--config <file>` | 指定 `collect.conf` |
| `-p/--plugin-dir <dir>` | 指定插件目录 |

生成内容示例：

```text
snapshot_20260510_163000/
  summary.json
  app_logs/
  dmesg.txt
  network.txt
  thread.txt
snapshot_20260510_163000.tar.gz
```

daemon 模式下也可以用 `SIGUSR1` 触发一次快照：

```bash
kill -USR1 <collect_pid>
```

### 停止运行

Collect 捕获标准信号进行优雅退出：
- `SIGINT` (Ctrl+C): 停止运行并清理资源
- `SIGTERM` (kill <pid>): 停止运行并清理资源
- `SIGUSR1`: 生成一次异常诊断快照

```bash
killall collect
```

---

## 3. 插件系统

Collect 通过插件加载机制扩展功能。插件分为 **读取插件 (Read)** 和 **写入插件 (Write)**。

### 可用插件列表

#### 采集类 (Read Plugins)
- **cpu**: 采集 CPU 使用率 (User, System, Idle 等)。
- **memory**: 采集内存使用情况 (Total, Used, Free, Cached 等)。
- **df**: 采集磁盘空间使用情况。
- **uptime**: 采集系统启动时间和平均负载。

#### 快照类 (Snapshot Plugins)
- **network**: 异常快照时采集网络状态。
- **dmesg**: 异常快照时保存内核日志。
- **thread**: 异常快照时采集目标进程线程状态。

#### 输出类 (Write Plugins)
- **json_writer**: 将采集数据格式化为 JSON 并输出 (通常配合 IPC 使用或写入文件)。
- **csv**: 将数据写入 CSV 文件。
- **logfile**: 将数据写入日志文件。

### 插件启用
`collect.conf` 中存在 `LoadPlugin` 时，`collect` 只加载这些插件，并将对应 `<Plugin name>` 块中的参数传递给插件的 `Configure()`。如果配置文件中没有任何 `LoadPlugin`，则兼容旧行为：加载指定插件目录下的所有 `.so` 文件。

---

## 4. 配置文件

Collect 支持通过 JSON 文件配置应用行为，默认路径为 `/etc/collect/user_config.json`。

`collect.conf` 可配置快照默认路径：

```text
SnapshotDir "/mnt/data/collect/snapshots"
AppLogDir "/mnt/data/app/logs"
SnapshotPack true
```

### 示例配置 `user_config.json`

```json
{
    "app": {
        "interval": 5,
        "log_level": "info"
    },
    "plugins": {
        "cpu": {
            "enabled": true
        },
        "network": {
            "interface": "eth0"
        }
    }
}
```

---

## 5. IPC 接口

Collect 提供基于 UNIX Domain Socket 的 IPC 接口，允许外部程序（如 APP 或上位机）与守护进程交互。

- **Socket 路径**: 默认为 `/tmp/collect.sock`
- **通信协议**: 自定义文本协议或 JSON (视 `IpcServer` 实现而定)

外部程序可以通过连接该 Socket 获取实时采集数据或发送控制指令。

---

## 6. 常见问题

**Q: 启动时提示 "No plugins found"？**
A: 请确保 `-p` 参数指向了正确的包含 `.so` 文件的目录。编译生成的插件通常在 `build/lib/collect/modules` 或 `build/plugins` 下。

**Q: 如何查看运行日志？**
A: Collect 默认将日志输出到标准输出 (stdout/stderr)。如果作为后台服务运行，建议重定向输出到日志文件。
