# Apple_Cat 4.0 (Qt 6 重构版)

File_Creater/Apple_Cat 的 Qt 6 重写。与旧版本不保证兼容，但完整实现了原功能
并满足全部七项重构需求。

## 构建

```
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.x/mingw_64
cmake --build build
windeployqt build/bin/AppleCat.exe     # 部署 Qt DLL（可选，仅分发时需要）
```

构建产物：`build/bin/AppleCat.exe`、`build/bin/plugins/libstatsplugin.dll`
（示例插件，自动被主程序加载）。

命令行：

```
AppleCat --selftest          # 45 项无头自检，报告写入 selftest_result.txt，退出码 0/1
AppleCat file1.txt file2.py  # 打开一个或多个文件
```

## 工程结构（前后端分离）

```
qt6/
├── interfaces/      # 插件 SDK（applecat_plugin.h，纯头文件，插件只依赖它）
├── core/            # 后端静态库，零控件依赖（QtCore/QtGui）
│   ├── appsettings.*        # 设置服务：默认值、可移植 INI、变更信号
│   ├── encodingservice.*    # 编码注册表 + BOM/启发式自动检测
│   ├── asyncio.*            # 每文件一线程的异步读写 + 进度回调
│   ├── syntaxrepository.*   # 语法定义加载（用户目录 > exe 目录 > 内嵌资源）
│   ├── syntaxhighlighter.*  # 规则驱动高亮器（关键字/注释/正则，跨块注释状态）
│   ├── completion.*         # 多源补全引擎 + 模糊排序 + 片段
│   └── selftest.*           # --selftest 测试套件
├── app/             # GUI 前端（QMainWindow + 文档视图）
│   ├── codeeditor.*         # 编辑器：行号、当前行高亮、自动缩进、括号补全
│   ├── completionpopup.*    # 补全弹窗（不抢键盘焦点）
│   ├── findreplacebar.*     # 非模态查找/替换栏（高亮全部、计数、正则、循环查找）
│   ├── editortab.*          # 文本标签页：异步加载、大文件分块填充、编码/BOM
│   ├── hexeditor.*          # 主窗口内嵌十六进制编辑器（可编辑、可撤销、可保存）
│   ├── settingsdialog.*     # 六页设置对话框（含插件管理页）
│   ├── pluginhost.*         # 插件加载器 + IAppContext 实现
│   └── mainwindow.*         # 主窗口：菜单、标签页、最近文件、拖放、关闭确认
└── plugins/statsplugin/     # 示例插件（菜单动作 + 补全提供者）
```

核心库（core）只依赖 QtCore/QtGui，不包含任何窗口控件；所有界面都在 app 中，
插件通过 interfaces 的抽象接口与宿主交互——这就是本项目的"前后端分离"。

## 七项需求对照

| # | 需求 | 实现 |
|---|------|------|
| 1 | 更智能、可扩展的文本补全 | `completion.h` 多源引擎：语言关键字（语法 JSON）+ 代码片段（snippets.json，支持 `${cursor}`/`${Name}` 占位符）+ 当前文档词（防抖缓存）+ **插件补全提供者**（ICompletionProvider）；前缀优先 + 可选模糊子序列匹配的排序，每源可独立开关，全在设置里配置 |
| 2 | HexViewer 在主界面打开、可编辑可保存 | `hexeditor.cpp`：十六进制编辑器是普通标签页（Ctrl+Shift+H），表格模型可编辑（字节列直接改），支持撤销栈、偏移跳转、字节数/行切换、ASCII 列显示，异步保存回原文件 |
| 3 | 查找/替换不阻塞主界面 | `findreplacebar.cpp`：查找替换改为编辑器内嵌的**非模态栏**（Ctrl+F/Ctrl+H），文档保持可编辑；全部匹配实时高亮（防抖 + 上限），计数标签（"3/5"），Enter/Shift+Enter 循环跳转，支持正则（含 `\1` 反向引用）与全部替换（单次撤销） |
| 4 | 更丰富的设置 | `settingsdialog.cpp` 六页：编辑器（字体/字号/Tab 宽/自动缩进/括号补全/换行/行号/当前行高亮）、颜色（6 项）、补全（8 项）、编码（默认打开/保存/检测回退）、十六进制、查找；另有插件管理页（启用/停用即时生效）。改动批量写入，信号驱动所有已开文档**实时生效**。设置支持可移植模式（exe 旁 settings.ini 优先） |
| 5 | 更多编码支持 | `encodingservice.cpp`：Unicode 家族（UTF-8/16/32、BOM 读写）+ 操作系统代码页（GBK、GB18030、Big5、Shift-JIS、EUC-JP、EUC-KR、KOI8-R/U、Windows-125x 全系、ISO-8859 全系等，本机实测 **31 种**，旧版仅 9 种）；自动检测：BOM → 无 BOM UTF-16 → 严格 UTF-8 → DBCS 候选 → 单字节回退。每个标签页可单独切换编码并重读，BOM 开关 |
| 6 | 每文件一线程加载，大文件不卡界面 | `asyncio.cpp`：每次加载/保存独占一个 QThread，结果经队列信号回 GUI 线程（接收者销毁自动断开）；`editortab.cpp` 对 >1MB 文档按 512KB 分块渐进填充（标签页标题显示百分比进度），>256MB 拒绝打开并提示 |
| 7 | 插件功能 | `interfaces/applecat_plugin.h` + `pluginhost.cpp`：插件为带 `Q_PLUGIN_METADATA` 的 DLL，实现 IPlugin（initialize/shutdown），通过 IAppContext 获得：自建菜单、注册菜单动作（SLOT 回调）、读当前文档、发状态栏消息、读写设置、注册补全提供者。插件放 exe 旁 `plugins/` 或 `%APPDATA%/AppleCat/plugins/`；设置对话框里可见状态并可启用/停用。`plugins/statsplugin/` 是完整示例 |

## 扩展点

- **新增语言**：把 `name/extensions/keywords/lineComment/blockComments/regexRules`
  格式的 JSON 丢进 `%APPDATA%/AppleCat/syntax/`（或 exe 旁 syntax/），无需重新编译。
- **新增片段**：同目录下 `snippets.json`，键为语言后缀或 `"*"`（全局）。
- **新插件**：编译为 DLL，链接 Qt 与 `interfaces/` 头文件，参考 statsplugin。

## 已知取舍

- 传统编码依赖操作系统代码页（Windows API），非 Windows 平台自动降级为
  ISO-8859-1 直通。
- 自动检测是启发式：优先 BOM/UTF-16/严格 UTF-8，再按 GBK→Big5→Shift-JIS→
  EUC-KR→单字节顺序严格校验，最后回退 UTF-8（非法序列替换显示）。
- HexEditor 的 ASCII 列目前为只读显示，编辑请用字节列（语义更明确）。

Origin: github.com/KamisatoAyaka7/File_Creater — developed by szy
