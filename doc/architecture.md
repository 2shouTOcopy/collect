# 🏗️ Collect 系统架构设计

> 本文档包含系统的组件关系、插件加载时序和核心数据流的可视化设计。

---

## 一、组件图 (Component Diagram)

展示 `PluginManager`、`ConfigManager`、`WriteQueue`（DataBus）和各类插件的依赖关系。

```mermaid
graph TB
    subgraph App["应用层"]
        CD["CollectDaemon<br/>(主事件循环)"]
    end

    subgraph Core["核心框架"]
        PM["PluginManager<br/>(插件注册 & 能力分类)"]
        DS["Dispatcher<br/>(最小堆调度器)"]
        WQ["WriteQueue<br/>(有界缓冲队列)"]
        PL["PluginLoader<br/>(.so 动态加载)"]
    end

    subgraph Config["配置系统"]
        CM["ConfigManager<br/>(collect.conf 解析)"]
        TD["TypesDb<br/>(数据类型定义)"]
    end

    subgraph Interact["交互层"]
        IPC["IpcServer<br/>(UNIX Domain Socket)"]
        ACM["AppConfigManager<br/>(user_config.json)"]
    end

    subgraph Output["输出格式"]
        JF["JsonFormatter<br/>(AI/LLM 友好)"]
        CF["CsvFormatter"]
    end

    subgraph ReadPlugins["Read 插件 (.so)"]
        CPU["cpu"]
        MEM["memory"]
        DF["df"]
        UP["uptime"]
        NET["network"]
        THR["thread"]
        DMG["dmesg"]
    end

    subgraph WritePlugins["Write 插件 (.so)"]
        CSV["csv_writer"]
        LOG["logfile_writer"]
        JSON["json_writer"]
    end

    %% 主控流
    CD -->|"驱动调度"| DS
    CD -->|"drain 写队列"| WQ
    CD -->|"poll IPC"| IPC

    %% 配置流
    CM -->|"插件目录/全局参数"| PL
    CM -->|"interval/TypesDB"| DS
    CM -->|"配置参数"| PM
    TD -.->|"DataSet 定义"| WQ

    %% 插件管理
    PL -->|"加载 .so"| ReadPlugins
    PL -->|"加载 .so"| WritePlugins
    PL -->|"返回 IPlugin*"| PM
    PM -->|"注册 read 回调"| DS
    PM -->|"注册 write 回调"| WQ

    %% 数据流
    DS -->|"调用 Read()"| ReadPlugins
    ReadPlugins -->|"DispatchValues()"| WQ
    WQ -->|"调用 Write()"| WritePlugins
    WritePlugins -->|"格式化"| JF
    WritePlugins -->|"格式化"| CF

    %% IPC 交互
    IPC -->|"配置命令"| ACM
    IPC -->|"状态查询"| PM

    %% 样式
    style CD fill:#4a90d9,color:#fff
    style DS fill:#e67e22,color:#fff
    style WQ fill:#e67e22,color:#fff
    style PM fill:#27ae60,color:#fff
    style PL fill:#27ae60,color:#fff
    style IPC fill:#8e44ad,color:#fff
    style ACM fill:#8e44ad,color:#fff
    style CM fill:#2c3e50,color:#fff
    style TD fill:#2c3e50,color:#fff
    style JF fill:#16a085,color:#fff
    style CF fill:#16a085,color:#fff
```

**组件职责说明**：

| 颜色 | 层级     | 组件                        |
| ---- | -------- | --------------------------- |
| 🔵 蓝 | 应用层   | CollectDaemon               |
| 🟠 橙 | 调度层   | Dispatcher, WriteQueue      |
| 🟢 绿 | 插件管理 | PluginManager, PluginLoader |
| 🟣 紫 | 交互层   | IpcServer, AppConfigManager |
| ⚫ 深 | 配置层   | ConfigManager, TypesDb      |
| 🟩 青 | 输出格式 | JsonFormatter, CsvFormatter |

---

## 二、插件加载时序图 (Sequence Diagram)

展示系统启动时如何通过动态库 (`.so`) 加载插件、配置并初始化。

```mermaid
sequenceDiagram
    autonumber
    participant Main as main()
    participant CD as CollectDaemon
    participant CM as ConfigManager
    participant PL as PluginLoader
    participant SO as plugin.so
    participant PM as PluginManager
    participant DS as Dispatcher
    participant IPC as IpcServer

    Main->>CD: configure(argc, argv)
    CD->>CM: Read("collect.conf")
    CM->>CM: 解析全局参数<br/>(Interval, PluginDir, TypesDB)

    rect rgb(240, 248, 255)
        Note over CM,SO: 插件加载阶段
        CM->>PL: SetDir(pluginDir)
        loop 每个 LoadPlugin 指令
            CM->>PL: Load("cpu", global=false)
            PL->>SO: dlopen("cpu/cpu.so")
            SO-->>PL: handle
            PL->>SO: dlsym("CreateModule")
            SO-->>PL: pfnCreateModule
            PL->>SO: CreateModule()
            SO-->>PL: IPlugin* 实例
            PL-->>PM: Register(plugin)
        end
    end

    rect rgb(255, 248, 240)
        Note over CM,PM: 插件配置阶段
        loop 每个 Plugin 配置块
            CM->>PM: DispatchBlockConfig("cpu", children)
            PM->>SO: plugin->Configure("ReportByCpu", "true")
            SO-->>PM: 0 (成功)
        end
    end

    rect rgb(240, 255, 240)
        Note over PM,DS: 插件初始化 & 注册阶段
        CD->>PM: InitAll()
        loop 每个已注册插件
            PM->>SO: plugin->Init()
            SO-->>PM: 0 (成功)
            alt HasRead() == true
                PM->>DS: RegisterRead(plugin, interval)
            end
            alt HasWrite() == true
                PM->>DS: RegisterWrite(plugin)
                Note right of DS: 加入 write 回调列表
            end
        end
    end

    CD->>IPC: Start(socketPath)
    CD->>CD: Loop() — 进入主事件循环

    rect rgb(255, 240, 240)
        Note over CD,SO: Init 失败处理
        PM->>SO: plugin->Init()
        SO-->>PM: -1 (失败)
        PM->>PM: 标记为 InitFailed
        PM->>PM: 指数退避后重试<br/>(10s → 20s → 40s → ...)
    end
```

---

## 三、核心数据流图

展示指标从 Read 插件采集 → WriteQueue 缓冲 → Write 插件输出的完整路径。

```mermaid
flowchart LR
    subgraph Sources["系统数据源"]
        S1["/proc/stat"]
        S2["/proc/meminfo"]
        S3["/proc/net/dev"]
        S4["statvfs()"]
        S5["/proc/uptime"]
    end

    subgraph ReadPhase["Read 阶段 (Dispatcher 调度)"]
        R1["cpu.Read()"]
        R2["memory.Read()"]
        R3["network.Read()"]
        R4["df.Read()"]
        R5["uptime.Read()"]
    end

    subgraph Transform["数据封装"]
        VL["ValueList 构造<br/>plugin + type + value<br/>+ timestamp + interval"]
    end

    subgraph Buffer["WriteQueue (有界缓冲)"]
        direction TB
        Q["环形队列<br/>容量: 1024"]
        FULL{"队列满?"}
        DROP["丢弃最旧 + WARNING"]
    end

    subgraph Lookup["DataSet 查找"]
        DS["ConfigManager<br/>.GetDataSetByName()"]
    end

    subgraph WritePhase["Write 阶段 (DrainBatch)"]
        W1["csv_writer.Write()"]
        W2["logfile_writer.Write()"]
        W3["json_writer.Write()"]
    end

    subgraph Format["输出格式化"]
        F1["CsvFormatter"]
        F2["JsonFormatter<br/>(AI/LLM 友好)"]
    end

    subgraph Targets["输出目标"]
        T1["CSV 文件"]
        T2["日志文件"]
        T3["JSON 文件 / stdout"]
        T4["→ AI/LLM 分析工具"]
    end

    %% 数据源 → Read
    S1 --> R1
    S2 --> R2
    S3 --> R3
    S4 --> R4
    S5 --> R5

    %% Read → ValueList
    R1 --> VL
    R2 --> VL
    R3 --> VL
    R4 --> VL
    R5 --> VL

    %% ValueList → Buffer
    VL -->|"DispatchValues()"| Q
    Q --> FULL
    FULL -->|"否"| Lookup
    FULL -->|"是"| DROP
    DROP -.->|"腾出空间"| Q

    %% Buffer → Write
    DS --> W1
    DS --> W2
    DS --> W3
    Lookup --> DS

    %% Write → Format → Target
    W1 --> F1 --> T1
    W2 --> T2
    W3 --> F2 --> T3
    T3 -->|"pipe / file"| T4

    %% 样式
    style Q fill:#e67e22,color:#fff
    style VL fill:#3498db,color:#fff
    style F2 fill:#16a085,color:#fff
    style T4 fill:#8e44ad,color:#fff
    style DROP fill:#e74c3c,color:#fff
```

**数据流关键路径**：

```
/proc/stat → cpu.Read() → ValueList{plugin:"cpu", type:"percent", value:23.5}
    → WriteQueue.enqueue()
    → [主循环 DrainBatch]
    → csv_writer.Write(ds, vl)   → CSV 文件
    → json_writer.Write(ds, vl)  → JsonFormatter → {"plugin":"cpu","value":23.5,...}
                                                   → stdout → AI/LLM 工具
```
