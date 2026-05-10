# Collect 重构进展文档

> **最后更新**: 2026-05-10
> **状态**: 🔄 重构中

---

## 状态总览

| Phase | 名称               | 状态 | 说明                                   |
| ----- | ------------------ | ---- | -------------------------------------- |
| 0     | 项目基础设施       | ✅    | CMake + GTest + 目录结构               |
| 1     | 数据类型现代化     | ✅    | CdTime, DataSet, ValueList             |
| 2     | 插件系统重构       | ✅    | IPlugin + 10 个插件适配                |
| 3     | 调度与数据管道     | ✅    | Dispatcher + WriteQueue + 超时告警     |
| 4     | App 交互与 AI 输出 | ✅    | IPC + AppConfig + JsonFormatter        |
| 5     | 依赖注入与测试     | ✅    | 消除单例 + 组合式 DI + 87 个测试       |
| 6     | 旧代码清理         | ✅    | daemon/, module/, oconfig/ 已删除      |
| 7     | 配置解析实现       | ✅    | ConfigParser + TypesDb + ConfigManager |
| 8     | 异常快照能力       | ✅    | SnapshotManager + snapshot 插件 + CLI/SIGUSR1 |

---

## 项目统计

| 指标        | 数值                                               |
| ----------- | -------------------------------------------------- |
| src/ 源文件 | ~30 个 (.h/.cpp)                                   |
| 插件        | 10 个 (4 read + 3 flush + 2 write + 1 json_writer) |
| 单元测试    | 12 个测试文件                                      |
| 集成测试    | 1 个 pipeline 测试                                 |
| 测试用例    | 87+ 个 (待验证)                                    |
| C++ 标准    | C++14                                              |
| 构建系统    | CMake 3.14+                                        |

---

## 后续可扩展方向

1. ~~**ConfigParser 实现** — `src/config/ConfigParser.cpp` 当前为 stub~~ ✅ 已完成
2. ~~**CsvFormatter 实现** — `src/output/CsvFormatter.cpp` 当前为 stub~~ ✅ 已完成
3. ~~**配置驱动插件加载** — `LoadPlugin` 白名单与 `<Plugin>` 参数派发尚未完全接入~~ ✅ 已完成
4. **跨编译支持** — 添加 ARM 交叉编译工具链文件
5. **Prometheus 输出** — 新增 prometheus_writer 插件
6. **集成测试扩展** — Linux 端到端 daemon 启动/停止与 snapshot 打包测试

---

## Phase 8: 异常快照能力 (2026-05-10)

### 完成内容

- 新增 `SnapshotContext`、`SnapshotRequest`、`SnapshotResult`
- 新增 `SnapshotManager`，负责创建快照目录、复制 App 日志、调用 snapshot 插件、生成 `summary.json`、可选打包
- `PluginManager` 支持 `HasSnapshot()` 分类与 `SnapshotAll()`
- `network`、`thread`、`dmesg` 改为 snapshot 插件，输出到本次快照目录
- 新增 CLI: `collect snapshot --reason ... --app-log-dir ... --out ...`
- daemon 模式支持 `SIGUSR1` 触发快照，signal handler 只设置 pending 标志
- macOS 开发构建支持插件 `.so` 输出和 Apple `dynamic_lookup`
- 新增 `test_snapshot_manager.cpp`，当前测试用例总数 117

---

## Phase 7: 配置解析实现 (2026-02-19)

### 完成内容

- 实现 `ConfigParser` — 完整 oconfig 格式解析（注释、key-value、`<Block>` 嵌套块、引号字符串）
- 实现 `TypesDb::Load()` — 解析 types.db 格式（`type ds_name:TYPE:min:max, ...`）
- 实现 `ConfigManager::Load()` — 整合 ConfigParser，提取 LoadPlugin/PluginDir/TypesDB/Interval/Plugin 块
- 集成到 `CollectDaemon::Configure()` — 启动时加载 collect.conf 和 types.db
- 新增单元测试: `test_config_parser.cpp`（17 cases）、`test_types_db.cpp`（11 cases）

### 变更文件

| 操作 | 文件                                                             |
| ---- | ---------------------------------------------------------------- |
| 修改 | `src/config/ConfigParser.h` — 新增私有 helpers + ParseString     |
| 修改 | `src/config/ConfigParser.cpp` — 完整 oconfig 解析实现            |
| 修改 | `src/config/TypesDb.cpp` — types.db 解析实现                     |
| 修改 | `src/config/ConfigManager.h` — 新增 LoadPlugins/PluginConfig API |
| 修改 | `src/config/ConfigManager.cpp` — 完整配置管理实现                |
| 修改 | `src/app/CollectDaemon.h` — 添加 ConfigManager/TypesDb 成员      |
| 修改 | `src/app/CollectDaemon.cpp` — Configure() 集成配置加载           |
| 新增 | `tests/unit/test_config_parser.cpp`                              |
| 新增 | `tests/unit/test_types_db.cpp`                                   |
| 修改 | `tests/CMakeLists.txt` — 添加新测试目标                          |
