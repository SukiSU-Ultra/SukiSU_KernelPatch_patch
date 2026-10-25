# SukiSU 内置 / LKM 统一 KPM 桥接

调用链：管理器或 ksud → 原有 `KSU_IOCTL_EXTENSION` / `KSU_EXTENSION_KPM` → SukiSU `do_kpm()` → 内核间 supercall → KP 的 KPM 加载器。

不再 hook SukiSU 的七个 stub 函数，也不再提供上一版的 root 用户态 magic syscall。两个工程必须同时更新并重新构建。仍需将 KP 修补到 boot；单独安装 SukiSU LKM 不会产生 KPM 加载器。

## 内部协议 v1

定义分别位于 KP `supercall.h` 和 SukiSU `extension/kpm/bridge.h`。

SukiSU 解析原生 arm64 `__arm64_sys_truncate`（依次尝试普通名、`.cfi_jt`、`.cfi`），以保存模式为 EL1h 的临时 `pt_regs` 直接调用。KP inline hook 相同入口，不经过 EL0 syscall dispatcher，因此 LKM 何时加载不影响桥接安装。

- x0：`0x53554b504d000002`，非用户地址的协议标识，不是密钥。
- x1：操作号，0 为 ABI 探测，1～7 对应既有 LOAD、UNLOAD、NUM、LIST、INFO、CONTROL、VERSION。
- x2～x5：操作参数。
- x6、x7：第二、第三 magic，必须同时匹配。
- x8：ABI 版本 2；x9～x30 为保留寄存器，必须为零。
- 返回值：探测返回 2；其他调用返回结果或负错误码。

三个 magic 都是公开协议标识，不是密钥，也不证明调用者身份。首标识匹配但后续标识、版本或保留寄存器不正确时，KP 返回 `EPROTO`，不解引用操作参数。未使用的操作参数非零返回 `EINVAL`。v2 不回退到 v1，KP 与 SukiSU 必须配套重新构建。

字符串和列表／信息／版本输出均为内核缓冲区。CONTROL 的可选输出以及 loader 的 reserved 仍按原加载器定义使用用户指针；现有 extension UAPI 没有这两个附加字段，因此当前用户态接口不暴露它们。

KP 在解引用操作参数前检查保存的模式必须为 EL1h，真实用户 syscall（包括 root 和 compat）不能通过此入口传递内核指针。用户权限继续由 SukiSU 原有 `manager_or_root` 检查负责。没有 magic 的普通 truncate 保持原行为。无 KP 或协议不匹配时，SukiSU 返回 `EOPNOTSUPP`；不缓存“不可用”结果。

## 用户态兼容

`ksu_kpm_cmd` 和 1～7 操作号未改变；管理器无需改用另一套 syscall。LIST 和 VERSION 按调用者容量写入带 NUL 的结果，容量上限 4096；LIST 空列表返回空字符串。INFO 的旧 ABI 没有容量字段，按当前 ksud 的 256 字节缓冲区输出，超长信息会截断。完整、可协商长度的 INFO 以及 ctl0 输出需要另行设计扩展 UAPI。

SukiSU extension GET 的 value 现在同时检查 CONFIG_KPM 和 KP 桥接是否实际可用。ksud 保留原 ioctl 路径，并修复负 errno 展示和无界字符串读取；加载失败不再伪装成功。
