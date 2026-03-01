# NeurolingsCE

**[English](README_EN.md) | 中文**

跨平台桌面看板娘（Shimeji）应用，基于 [Shijima-Qt](https://github.com/pixelomer/Shijima-Qt) 深度修改而来。

使用 C++17 / Qt6 构建，支持 Windows、Linux 和 macOS。

## 特性

- 🖥️ 跨平台支持（Windows / Linux / macOS）
- 🎭 兼容 Shimeji-ee 格式的看板娘资源包
- 📦 拖放导入看板娘压缩包
- 🪟 窗口模式 — 在独立沙盒窗口中运行看板娘
- 🖱️ 鼠标交互 — 拖拽、右键菜单
- 📡 HTTP REST API（`localhost:32456`）
- 🌐 多语言支持（English / 中文简体）
- 🔊 可选的音效支持（Qt Multimedia）
- 🖥️ 多显示器支持
- 📐 自定义缩放

## 下载

- [最新版本](https://github.com/qingchenyouforcc/NeurolingsCE/releases/latest)
- [所有版本](https://github.com/qingchenyouforcc/NeurolingsCE/releases)

## 文档

📖 **[Wiki 文档](https://github.com/qingchenyouforcc/NeurolingsCE/wiki)** — 包含快速开始、构建指南、架构说明、HTTP API、常见问题等完整文档。

## 构建

### 前置依赖

- C++17 编译器（MSVC 2022 / GCC / Clang）
- Qt 6.8+（Core, Gui, Widgets, Concurrent, LinguistTools）
- CMake 3.21+（Windows/MSVC）或 Make（Linux/macOS）

子模块需要初始化：

```bash
git submodule update --init --recursive
```

### Windows（MSVC + CMake）

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQt6_DIR=D:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6
cmake --build build
```

也可以直接用 Visual Studio 打开项目，在 `CMakeSettings.json` 中已配置好 `x64-Debug` 和 `x64-Release` 两个方案。

### Windows（MinGW 交叉编译 via Docker）

```bash
docker build -t neurolingsce-dev dev-docker
docker run -e CONFIG=release --rm -v "$(pwd)":/work neurolingsce-dev bash -c 'mingw64-make -j$(nproc)'
```

### Linux

安装 Qt6 开发依赖后：

```bash
CONFIG=release make -j$(nproc)
```

### macOS

1. 安装 MacPorts：

```bash
sudo port install qt6-qtbase qt6-qtmultimedia pkgconfig libarchive
```

2. 构建：

```bash
CONFIG=release make -j$(nproc)
```

## 平台说明

### Windows

仅支持 x64 工具链。已在 Windows 11 上测试，Windows 10 应该也可以工作。窗口追踪开箱即用。

### Linux

支持 KDE Plasma 6 和 GNOME 46（Wayland / X11）。首次运行时会自动安装 shell 插件来获取前台窗口信息：
- **KDE** — 对用户透明，无需操作。
- **GNOME** — 首次运行后需要重新登录以重启 Shell。程序会给出相应提示。
- **其他桌面环境** — 窗口追踪不可用。

### macOS

需要辅助功能（Accessibility）权限来获取前台窗口。最低系统版本 macOS 13。

## HTTP API

内置 HTTP REST API 运行在 `http://127.0.0.1:32456`，可用于外部程序控制看板娘。

详细文档见 [src/docs/HTTP-API.md](src/docs/HTTP-API.md)。

## 项目结构

```
NeurolingsCE/
├── src/app/              # Qt 应用层
├── src/platform/Platform/ # 平台抽象层（Windows/Linux/macOS）
├── include/shijima-qt/   # 公共头文件
├── libshijima/           # [子模块] 核心看板娘模拟引擎
├── libshimejifinder/     # [子模块] 看板娘资源包导入/解压
├── cpp-httplib/          # [子模块] HTTP 服务器（header-only）
├── translations/         # i18n 翻译文件
├── cmake/                # CMake 辅助脚本
├── src/assets/           # 内置默认看板娘资源
└── src/packaging/        # 桌面入口、图标、AppStream 元信息
```

## 致谢

本项目基于 [pixelomer](https://github.com/pixelomer) 的 [Shijima-Qt](https://github.com/pixelomer/Shijima-Qt) 开发，在此基础上进行了大量修改和功能增强。

核心依赖：
- [libshijima](https://github.com/pixelomer/libshijima) — 看板娘模拟引擎
- [libshimejifinder](https://github.com/pixelomer/libshimejifinder) — 资源包解析
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) — HTTP 库
- [Qt 6](https://www.qt.io/) — GUI 框架

## 联系方式

- **作者**：[轻尘呦](https://space.bilibili.com/178381315)
- **项目地址**：https://github.com/qingchenyouforcc/NeurolingsCE
- **问题反馈**：[GitHub Issues](https://github.com/qingchenyouforcc/NeurolingsCE/issues)
- **反馈 QQ 群**：423902950
- **交流 QQ 群**：125081756

**如果你对neuro社区项目开发感兴趣的话**
  
**可以联系我加入NeuForge Center**

**请加入STNC了解更多内容**

**STNC蜂群技术情报中心QQ群 125081756**

**STNC项目反馈QQ群 423902950**

## 许可证

本项目基于 [GNU General Public License v3.0](LICENSE) 开源。

上游项目 Shijima-Qt 的 README 见 [Shijima-Qt_README.md](Shijima-Qt_README.md)。

![](https://qingchenyou-1301914189.cos.ap-beijing.myqcloud.com/681dcdd42da7fc5484c1dd3a9875b54a_324.png)

---
## 广告位

如果你对neuro同人文感兴趣的话，请加入文学社谢谢喵

**NeuroEcho文学社QQ群1063898428**

请加入NeuroSama吧群谢谢喵

**NeuroSama吧QQ群734688012**

**EvilNeuro吧QQ群1072864404**

---

## Star 趋势

[![Star History Chart](https://api.star-history.com/svg?repos=qingchenyouforcc/NeurolingsCE&type=Date)](https://star-history.com/#qingchenyouforcc/NeurolingsCE&Date)
