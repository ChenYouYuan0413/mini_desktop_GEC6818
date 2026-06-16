# mini_desktop — GEC6818 桌面系统

粤嵌 GEC6818（ARM Cortex-A53 armv7l, Linux 3.4.39-gec）
嵌入式桌面系统。800×480 LCD 触摸屏，集成电子相册、红外遥控和游戏启动器。

## 功能

```
桌面 ── 图库     （触摸滑动 / 遥控翻页浏览照片）
    ├─ 游戏     （3D 光线投射游戏）
    └─ 红外     （学习 / 发送 / 接收红外遥控码）
            └─ 后台接收 → 遥控桌面导航
```

- **桌面**：3 个圆角图标（图库、游戏、红外），支持触摸点击和遥控框选
- **图库**：BMP 照片浏览，触摸左右滑动或遥控翻页，上键/点顶栏退出
- **游戏**：通过 `system()` 启动外部 `raycast_game_static`，退出后 fb_reset 恢复
- **红外**：5 键学习（上下左右中），发送/接收页面，后台常驻识别遥控码
- **遥控导航**：左右键移动白色选框，中键进入应用

## 硬件

| 项目 | 详情 |
|------|------|
| 开发板 | 粤嵌 GEC6818 |
| 主控 | Samsung S5P6818, ARM Cortex-A53, armv7l |
| 系统 | Linux 3.4.39-gec, BusyBox |
| 屏幕 | 800×480 LCD, 32bpp framebuffer (/dev/fb0) |
| 触摸 | gslX680, 1024×600 raw → 800×480 校准 (/dev/input/event0) |
| 红外 | YS-IRTM 红外模块，串口 /dev/ttySAC1, 9600 8N1, 模块自动去 A1F1 头，输出 3 字节用户码 |
| 蜂鸣器 | /sys/kernel/gec_ctrl/beep（4 字节小端 int 写入；shell echo 会写入 `0\n` 误解析为非零值）|
| LED | /sys/class/leds/ledN/brightness（1 字节 ASCII '0'/'1' 写入）|

## 项目结构

```
mini_desktop/
├── main.c                  # 入口
├── Makefile
├── common/                 # 硬件抽象层
│   ├── device.c/.h         # framebuffer / 触摸 / BMP / 字体 / 蜂鸣器 / LED
│   ├── param.h             # 屏幕参数、触摸校准
│   └── tools.c/.h
├── button/                 # 按钮系统
│   └── button.c/.h         # 圆角矩形、文字标签、BMP 背景、绿色已学标记
├── album/                  # 电子相册
│   └── album.c/.h
├── ir_app/                 # 红外功能
│   └── ir_app.c/.h         # 学习 / 发送 / 接收 / 后台遥控
├── desktop/                # 桌面
│   └── desktop.c/.h        # 图标布局、触摸分发、遥控导航
├── fonts/
│   └── font_16x16.h        # 16×16 等宽字模（A-Z + 0-9, Monaco 14pt 生成）
├── images/                 # 相册照片（800×480 BMP）
├── icon/                   # 桌面图标 + 背景（BMP）
└── test/                   # 分步测试
```

## 板子初始化

首次拿到板子或烧录系统后，需要配置网络和 SSH。

### 固定 IP

编辑 `/etc/profile`，在末尾添加：

```bash
ifconfig eth0 up
ifconfig eth0 192.168.1.88 netmask 255.255.255.0
route add default gw 192.168.1.1
```

重启或执行 `source /etc/profile` 生效。注意板子 IP 需要和电脑在同一网段。

### 启用 eth0

如果 `ifconfig` 看不到 eth0，检查网线是否插好、交换机/路由器是否通电：

```bash
# 查看所有网口
ifconfig -a

# 手动启用
ifconfig eth0 up
```

某些镜像的 eth0 默认是 down 状态，加到 `/etc/profile` 里开机自动启用即可。

### 安装 SSH（dropbear）

板子默认可能没有 SSH 服务。检查：

```bash
ls /usr/sbin/dropbear   # dropbear 服务端
ls /usr/bin/dbclient    # dropbear 客户端（SSH 登录用）
```

如果缺失，从交叉编译工具链或同型号板子拷贝：

```bash
# 在能 SSH 的板子上导出
scp /usr/sbin/dropbear /usr/bin/dbclient root@<目标IP>:/tmp/

# 在目标板子上
cp /tmp/dropbear  /usr/sbin/dropbear
cp /tmp/dbclient  /usr/bin/dbclient
chmod +x /usr/sbin/dropbear /usr/bin/dbclient
```

创建 host key 并启动：

```bash
mkdir -p /etc/dropbear
dropbearkey -t rsa -f /etc/dropbear/dropbear_rsa_host_key
dropbearkey -t dss -f /etc/dropbear/dropbear_dss_host_key
dropbear -p 22
```

确认启动：

```bash
ps | grep dropbear
```

设置开机自启（加到 `/etc/profile` 末尾）：

```bash
dropbear -p 22 &
```

之后即可从电脑 SSH 登录：`ssh root@192.168.1.88`。

## 编译

```bash
docker run --platform linux/amd64 --rm \
  -v /path/to/toolchain:/opt/gec6818-toolchain:ro \
  -v /path/to/mini_desktop:/work -w /work \
  gec6818-dev:ubuntu18 bash -c 'make static'
```

工具链路径：[`gec6818_env_toolchain/toolchain/`](../gec6818_env_toolchain/toolchain/)，ARM 32-bit soft-float，GCC 5.4.0。

## 部署

板子 IP `192.168.1.88`，目标目录 `/home/cyy/`。

```bash
# 停止旧进程
ssh root@192.168.1.88 'killall mini_desktop 2>/dev/null; sleep 1'

# 主程序
scp -O mini_desktop root@192.168.1.88:/home/cyy/

# 资源文件
ssh root@192.168.1.88 'mkdir -p /home/cyy/icon /home/cyy/images'
scp -O icon/*.bmp   root@192.168.1.88:/home/cyy/icon/
scp -O images/*.bmp root@192.168.1.88:/home/cyy/images/

# 游戏（单独部署）
scp -O raycast_game_static root@192.168.1.88:/home/cyy/
```

### 需要部署的 BMP 清单

```
icon/
├── background.bmp    # 桌面背景（800×480）
├── photo_icon.bmp    # 图库图标（~80×80，圆角）
├── game_icon.bmp     # 游戏图标
├── ir_icon.bmp       # 红外图标
├── bg_page.bmp       # 红外子页面背景（800×480）
└── bar_top.bmp       # 红外顶栏（800×44）

images/
├── photo_1.bmp       # 相册照片（800×480）
├── photo_2.bmp
└── photo_3.bmp
```

IR 学习数据 `ir_send_*.txt` / `ir_recv_*.txt` 由程序自动生成，无需预部署。

## 运行

在板子上：

```bash
cd /home/cyy
./mini_desktop
```

触摸顶栏返回上级菜单，触摸图标进入应用。遥控器需先在「红外 → Learn → RECV 模式」学习 5 个键（上下左右中），之后桌面和图库中可用遥控操作。

## 设计约定

### 非阻塞 I/O

所有 I/O 使用 `O_NONBLOCK` + 事件排空模式。每帧先 `while(ts_read > 0)` 排空触摸，
循环退出后执行其他逻辑（串口轮询等），最后回到外层循环。UI 对触摸和串口同时响应。

### `ts_read()` 的 `has_new` 陷阱

`ts_read()` 内有 `static int has_new`，**每次调用必须重置为 0**。
忘记重置会导致首次触摸后 `ts_read()` 永远返回 1，
`while(ts_read > 0)` 死循环，后续代码永不执行。
此前曾导致红外学习页面无法读到串口数据。

### 触摸排空

子应用返回时务必排空触摸 + 红外缓冲，防止上一个触摸/遥控事件泄漏到下个页面。
使用 `drain_both()`（排空触摸 + `ir_flush()`）。

### 游戏退出后的 framebuffer

游戏使用双缓冲 `FBIOPAN_DISPLAY` 翻页，退出后 yoffset 可能非零。
必须在游戏返回后调 `fb_reset()` 复位 yoffset 并重读参数。

### BMP 渲染性能

- `bmp_display`（全屏背景）：stride 匹配时一次 `memcpy`，否则逐行 `memcpy`
- `show_bmp`（图标/局部）：无裁剪时一次 `memcpy`，有裁剪时逐行 `memcpy`
- 不再逐像素处理透明（alpha 字节由 LCD 控制器忽略）
