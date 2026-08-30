# PKE 第一至第六章编辑覆盖台账

## 审计范围

本台账以 Git commit `c30acbe` 中的六章为原始基线，记录方案 C 教学改写后的逐节覆盖情况。改写采用“在原完整材料前建立模型和因果链”的方式，原文中的实验要求、TODO、代码、命令、输出、图片、链接、参考答案和更多指导均保留。

机器审计由 `tools/verify_pke_docs.py` 完成，检查编号标题、HTML 锚点、fenced code body、图片目标、链接目标、URL、Markdown fence 和本地资源。人工审计逐节对照原文与改写后的新增教学层，确认关键事实仍位于原编号节中。

状态说明：

- **已核对**：已直接对照基线原节；原材料保留，新增教学结构与该节内容一致；
- **原位**：原编号标题和锚点未迁移；
- **基础结构**：问题 → 心智模型 → 具体流程 → 代码连接 → 边界/错误；
- **Lab 结构**：当前行为 → 目标 → 调用链/数据流 → 实现推理 → 验证 → 原指导/答案。

## 第一章：RISC-V 体系结构

| 基线编号与标题 | 改写位置 | 教学结构与重点 | 人工语义检查 |
| --- | --- | --- | --- |
| 1.1 RISC-V发展历史 | 原位 `#history` | 基础结构；区分开放 ISA 与具体实现 | 已核对 |
| 1.2 RISC-V汇编语言 | 原位 `#assembly` | 基础结构；指令读写、ABI、调用栈和寄存器时刻 | 已核对 |
| 1.3 机器的特权状态 | 原位 `#machinestates` | 基础结构；M/S/U 权限、CSR 与越权异常 | 已核对 |
| 1.4 中断和中断处理 | 原位 `#traps` | 基础结构；统一 trap 事件记录和 PC 规则 | 已核对 |
| 1.5 页式虚存管理 | 原位 `#paging` | 基础结构；VPN/PTE/PA/offset 与动作分类 | 已核对 |
| 1.6 相关工具软件 | 原位 `#toolsoftware` | 基础结构；编译、链接、装载、模拟、调试流水线 | 已核对 |

## 第二章：实验环境配置与实验构成

| 基线编号与标题 | 改写位置 | 教学结构与重点 | 人工语义检查 |
| --- | --- | --- | --- |
| 2.1 操作系统部分实验环境安装 | 原位 `#environments` | 基础结构；当前/目标行为与环境验收证据 | 已核对 |
| 2.1.1 安装开发环境 | 原位 `#subsec_osenvironments` | 决策结构；WSL/Linux、Docker、头歌路线选择 | 已核对 |
| 2.1.2 安装支撑软件 | 原位 `#subsec_softwarepackages` | 基础结构；host/target、目的/预期/诊断/下一步 | 已核对 |
| 2.1.3 头歌平台 | 原位 `#subsec_educoder` | 基础结构；页面、容器、评测与仓库继承边界 | 已核对 |
| 2.2 riscv-pke代码的获取 | 原位 `#preparecode` | 工作流结构；目录、分支、status、make、Spike | 已核对 |
| 2.3 PKE实验的组成 | 原位 `#pke_experiemnts` | 依赖结构；基础实验主线与挑战实验分叉 | 已核对 |

## 第三章：系统调用、异常和外部中断

| 基线编号与标题 | 改写位置 | 教学结构与重点 | 人工语义检查 |
| --- | --- | --- | --- |
| 3.1 实验1的基础知识 | 原位 `#fundamental` | 基础结构；源码到用户执行再到 HTIF 的完整启动链 | 已核对 |
| 3.1.1 RISC-V程序的编译和链接 | 原位 | 基础结构；目标文件、链接脚本和两个 ELF | 已核对 |
| 3.1.2 指定符号的逻辑地址 | 原位 | 基础结构；`st_value/st_size/st_name` 与字符串表 | 已核对 |
| 3.1.3 代理内核的构造过程 | 原位 | 基础结构；C、汇编、库和链接布局的产物链 | 已核对 |
| 3.1.4 代理内核的启动过程 | 原位 | 基础结构；模式、参数、栈、CSR 的启动追踪 | 已核对 |
| 3.1.5 ELF文件（app）的加载过程 | 原位 | 基础结构；可装载段与符号/调试 section 的边界 | 已核对 |
| 3.1.6 spike的HTIF接口 | 原位 | 基础结构；请求锁、一次初始化和 barrier 边界 | 已核对 |
| 3.2 lab1_1 系统调用 | 原位 `#syscall` | Lab 结构；`ecall`、`do_syscall`、`a0` 写回、PC 前移 | 已核对 |
| 3.3 lab1_2 异常处理 | 原位 `#exception` | Lab 结构；非法指令、cause 分派与异常循环 | 已核对 |
| 3.4 lab1_3（外部）中断 | 原位 `#irq` | Lab 结构；`SIP_STIP`、下一触发点和 PC 保持 | 已核对 |
| 3.5 challenge1 打印用户程序调用栈 | 原位 `#lab1_challenge1_backtrace` | Lab 结构；用户 fp/ra 链与 ELF 符号 | 已核对 |
| 3.6 challenge2 打印异常代码行 | 原位 `#lab1_challenge2_errorline` | Lab 结构；异常 PC、line table 与 host 源文件 | 已核对 |
| 3.7 challenge3 多核启动及运行 | 原位 `#lab1_challenge3_multicore` | Lab 结构；per-hart 状态、两类 barrier、timer、shutdown | 已核对 |

## 第四章：内存管理

| 基线编号与标题 | 改写位置 | 教学结构与重点 | 人工语义检查 |
| --- | --- | --- | --- |
| 4.1 实验2的基础知识 | 原位 `#fundamental` | 基础结构；一条 VA→PTE→PA+offset 主线 | 已核对 |
| 4.1.1 Sv39虚地址管理方案回顾 | 原位 `#sv39` | 基础结构；多级选择、叶子 PTE 与查询/创建模式 | 已核对 |
| 4.1.2 物理内存布局与规划 | 原位 `#physicalmemory` | 基础结构；物理页所有权、映射和回收顺序 | 已核对 |
| 4.1.3 PKE内核和应用逻辑地址空间 | 原位 `#virtualaddressspace` | 基础结构；区间、权限、每进程页表 | 已核对 |
| 4.1.4 页表操作函数 | 原位 `#pagetablecook` | 基础结构；walk/map/unmap/translate/allocator 副作用 | 已核对 |
| 4.2 lab2_1 虚实地址转换 | 原位 `#lab2_1_pagetable` | Lab 结构；传入页表、只读 walk、PTE PA 加 offset | 已核对 |
| 4.3 lab2_2 简单内存分配和回收 | 原位 `#lab2_2_allocatepage` | Lab 结构；分配、映射、取旧 PA、unmap、free | 已核对 |
| 4.4 lab2_3 缺页异常 | 原位 `#lab2_3_pagefault` | Lab 结构；store page fault、`stval`、补页并重试 | 已核对 |
| 4.5 challenge1 复杂缺页异常 | 原位 `#lab2_challenge1_pagefault` | Lab 结构；进程区域/权限判定与非法访问拒绝 | 已核对 |
| 4.6 challenge2 堆空间管理 | 原位 `#lab2_challenge2_singlepageheap` | Lab 结构；小块与承载页的两层生命周期 | 已核对 |
| 4.7 challenge3 多核内存管理 | 原位 `#lab2_challenge3_multicoremem` | Lab 结构；每进程页表、原子锁、per-hart 状态和退出 | 已核对 |

## 第五章：进程管理

| 基线编号与标题 | 改写位置 | 教学结构与重点 | 人工语义检查 |
| --- | --- | --- | --- |
| 5.1 实验3的基础知识 | 原位 `#fundamental` | 基础结构；保存上下文、选择进程、恢复状态 | 已核对 |
| 5.1.1 多任务环境下进程的封装 | 原位 | 基础结构；process 作为 trapframe/pagetable 所有权边界 | 已核对 |
| 5.1.2 进程的启动与终止 | 原位 | 基础结构；组装资源、退出状态与延迟回收 | 已核对 |
| 5.1.3 就绪进程的管理与调度 | 原位 | 基础结构；READY/RUNNING/BLOCKED 与队列一致性 | 已核对 |
| 5.2 lab3_1 进程创建（fork） | 原位 `#lab3_1_naive_fork` | Lab 结构；父子返回值、child pagetable/mapped info | 已核对 |
| 5.3 lab3_2 进程yield | 原位 `#lab3_2_yield` | Lab 结构；RUNNING→READY、尾入头出、不丢进程 | 已核对 |
| 5.4 lab3_3 循环轮转调度 | 原位 `#lab3_3_rrsched` | Lab 结构；timer、时间片和 FIFO ready queue | 已核对 |
| 5.5 challenge1 进程等待和数据段复制 | 原位 `#lab3_challenge1_wait` | Lab 结构；wait 阻塞/唤醒、代码共享、数据私有 | 已核对 |
| 5.6 challenge2 实现信号量 | 原位 `#lab3_challenge2_semaphore` | Lab 结构；count、专属等待队列和原子状态迁移 | 已核对 |
| 5.7 challenge3 写时复制 | 原位 `#lab3_challenge3_cow` | Lab 结构；双边只读 PTE、COW fault、复制与引用计数 | 已核对 |

## 第六章：文件系统

| 基线编号与标题 | 改写位置 | 教学结构与重点 | 人工语义检查 |
| --- | --- | --- | --- |
| 6.1 实验4的基础知识 | 原位 `#fundamental` | 基础结构；pathname→dentry→vinode→dinode→block | 已核对 |
| 6.1.1 文件系统概述 | 原位 | 基础结构；命名、存储、生命周期与目录本质 | 已核对 |
| 6.1.2 PKE的文件系统架构 | 原位 | 基础结构；VFS 统一接口、hostfs/RFS 分派 | 已核对 |
| 6.1.3 文件系统提供的接口 | 原位 | 基础结构；用户参数、VFS 对象与返回契约 | 已核对 |
| 6.1.4 虚拟文件系统 | 原位 | 基础结构；VFS 运行时层总览 | 已核对 |
| 6.1.4.1 VFS的功能接口 | 原位 | 基础结构；通用检查后进入 viop | 已核对 |
| 6.1.4.2 VFS的重要数据结构 | 原位 | 基础结构；dentry/vinode/file/superblock 职责 | 已核对 |
| 6.1.4.3 viop函数的定义和使用 | 原位 | 基础结构；`vinode->i_ops` 稳定分派点 | 已核对 |
| 6.1.4.4 VFS层的目录组织 | 原位 | 基础结构；dentry 树与盘上目录项分层 | 已核对 |
| 6.1.4.5 VFS层的哈希缓存 | 原位 | 基础结构；身份复用、一致性与真相来源 | 已核对 |
| 6.1.5 RFS文件系统 | 原位 | 基础结构；RFS 持久层总览 | 已核对 |
| 6.1.5.1 RFS磁盘结构和格式化 | 原位 | 基础结构；块号、元数据区和 `R_FREE/R_FILE/R_DIR` | 已核对 |
| 6.1.5.2 RFS的dinode和目录项 | 原位 | 基础结构；inum、写回持久化和 readdir offset | 已核对 |
| 6.1.5.3 硬链接 | 原位 | 基础结构；多个名字、一个 inode、nlinks/ref 边界 | 已核对 |
| 6.2 lab4_1 文件 | 原位 `#lab4_1_file` | Lab 结构；rfs_create、dinode 写回、vinode 初始化 | 已核对 |
| 6.3 lab4_2 目录文件 | 原位 `#lab4_2_dir` | Lab 结构；dir cache、offset 迭代和 R_DIR 创建 | 已核对 |
| 6.4 lab4_3 硬链接 | 原位 `#lab4_3_hardlink` | Lab 结构；新 direntry、旧 inode、nlinks 持久化 | 已核对 |
| 6.5 challenge1 相对路径 | 原位 `#lab4_challenge1_pwd` | Lab 结构；root/cwd 起点、`.`/`..` 逐分量查找 | 已核对 |
| 6.6 challenge2 重载执行 | 原位 `#lab4_challenge2_exec` | Lab 结构；VFS 读取 ELF、替换映像、重置 trapframe | 已核对 |
| 6.7 challenge3 简易Shell | 原位 `#lab4_challenge3_shell` | Lab 结构；read/parse、fork、exec、argv 栈、wait | 已核对 |
| 6.8 challengeX Put it all together | 原位 | Lab 结构；机制集成、多块文件和管道扩展 | 已核对 |

## 改写结果摘要

- 六章原始规模：9,078 行；原始材料未删除，新增教学层按章节原位插入；
- 每章均包含学习目标、前置知识、学习路线和完成问题；
- 所有基础节使用机制因果结构，所有 Lab/挑战使用任务驱动结构；
- 最终机器审计结果见 `editorial-audit.json`；
- 文档改写未修改 PKE 内核、用户程序、Makefile 或二进制资源，也未添加 OS 测试。
