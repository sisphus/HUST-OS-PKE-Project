# Lab 2 Challenge 2：单页小块堆分配设计

- 状态：待用户评审
- 日期：2026-08-27
- 范围：单进程、一个 4KB 用户堆页、可复用的小块分配
- 验证约束：不添加测试，只编译并运行课程给定的 `app_singlepageheap`

## 1. 背景和目标

当前 `lab2_3_pagefault` 继承的 `sys_user_allocate_page()` 每次申请都分配并映射一个完整的 4KB 物理页。该策略无法满足 Challenge 2：`better_malloc(100)` 和 `better_malloc(50)` 必须在同一个物理页内分配，释放第一个块后，下一次 `better_malloc(50)` 还应复用第一个块的虚拟地址。

目标是实现一个面向当前单进程的最小小块分配器：

1. 第一次申请时惰性分配并映射一个 4KB 用户堆页；
2. 在这一个页面内按申请大小分配连续小块；
3. 释放小块时只释放块，不解除仍被其他块使用的整页映射；
4. 使用 first-fit、块拆分和相邻空闲块合并；
5. 当整页没有活动块时，解除映射并回收物理页。

## 2. 非目标和约束

- 不实现多进程之间的堆隔离；当前 `process` 只有一个用户应用实例，但堆状态仍放在 `process` 中；
- 不实现跨多个物理页的堆扩展；申请大小必须不超过一个页面；
- 不实现完整 POSIX `malloc` 语义、线程安全或并发分配；
- 不在用户堆页内保存分配器元数据；
- 不新增自动化测试或测试应用；只运行课程给定应用进行验证。

## 3. 设计概览

### 3.1 地址模型

用户堆只使用一页虚拟地址：

```text
用户虚拟地址范围：[USER_FREE_ADDRESS_START,
                  USER_FREE_ADDRESS_START + PGSIZE)
```

该虚拟页只建立一次到物理页 `page_pa` 的映射。小块通过页内偏移区分：

```text
user_pointer = USER_FREE_ADDRESS_START + block.offset
```

因此用户指针是虚拟地址，物理页地址只由内核堆状态保存和使用。

### 3.2 元数据位置

在 `process` 中加入用户堆状态：

```text
user_heap {
    page_pa
    head
    blocks[512]
}

heap_block {
    offset
    size
    is_free
    active
    next
}
```

块描述符存放在内核进程状态中，不占用用户堆页面，也不能被用户程序覆盖。`512` 个描述符足以覆盖一个 4096 字节页面中按 8 字节最小粒度切分出的最多块数。

## 4. 组件和接口

### 4.1 用户库

保留现有接口：

```c
void *better_malloc(int n);
void better_free(void *va);
```

当前 [`user_lib.c`](../../../user/user_lib.c#L54-L64) 已经分别把 `n` 和 `va` 放入系统调用参数 `a1`，不需要改变用户 API。

### 4.2 系统调用层

保留现有系统调用号，调整内核函数使用参数：

```text
SYS_user_allocate_page(a1=n)
    → sys_user_allocate_page(n)
    → heap_alloc(current, n)

SYS_user_free_page(a1=va)
    → sys_user_free_page(va)
    → heap_free(current, va)
```

系统调用层只负责参数转发，不直接操作块链表。现有入口位于 [`syscall.c`](../../../kernel/syscall.c#L42-L74)。

### 4.3 堆管理器

新增一个内核堆管理模块，提供：

```c
void *heap_alloc(process *proc, uint64 n);
void heap_free(process *proc, uint64 va);
```

堆管理器负责块描述符、first-fit、拆分、合并，以及在需要时调用物理内存和虚拟内存接口。

### 4.4 虚拟内存层

复用现有 [`user_vm_map()`](../../../kernel/vmm.c#L173-L176) 和 [`user_vm_unmap()`](../../../kernel/vmm.c#L183-L201)：

- 第一次分配时映射整页；
- 释放单个小块时不调用 `user_vm_unmap()`；
- 所有块释放后才解除整页映射并回收物理页。

## 5. 分配算法

### 5.1 请求规范化

分配器拒绝 `n <= 0` 和 `n > PGSIZE` 的请求。有效请求向上对齐到 8 字节：

```text
aligned_size = ROUNDUP(n, 8)
```

### 5.2 第一次申请

当 `proc->heap.page_pa == 0` 时：

1. 调用 `alloc_page()` 获取物理页；
2. 将 `USER_FREE_ADDRESS_START` 映射到该物理页；
3. 创建一个 `[offset=0, size=PGSIZE, is_free=true]` 的空闲块；
4. 继续使用 first-fit 从该空闲块切出请求。

### 5.3 first-fit 和拆分

按 `offset` 从低到高遍历块链表，选择第一个满足 `is_free && size >= aligned_size` 的块。

如果剩余空间至少还能形成一个 8 字节块，则拆分为：

```text
原空闲块：[offset, size]

已分配块：[offset, aligned_size]
空闲块：  [offset + aligned_size,
           size - aligned_size]
```

如果剩余空间不足 8 字节，则整个空闲块分配出去，避免产生不可用碎片。

## 6. 回收算法

`heap_free(proc, va)` 首先检查 `va` 是否位于该堆页内，并且是否精确匹配一个活动块的起始地址。无效指针和重复释放不修改堆状态。

找到块后：

1. 将 `is_free` 设置为真；
2. 如果后继块空闲，则合并当前块和后继块；
3. 如果前驱块空闲，则合并前驱块和当前块；
4. 如果链表最终只剩一个覆盖整页的空闲块，则调用 `user_vm_unmap(page_dir, USER_FREE_ADDRESS_START, PGSIZE, 1)` 回收整页并重置堆状态。

关键性质：只要仍存在一个活动块，就不释放物理页，也不删除用户页表映射。

## 7. 给定应用的数据流

```text
better_malloc(100)
    → 分配 [0, 104)

better_malloc(50)
    → 分配 [104, 160)

better_free(m)
    → [0, 104) 标为空闲
    → [104, 160) 仍然有效
    → 整页映射保持不变

better_malloc(50)
    → first-fit 复用 [0, 104)
    → n == m
```

## 8. 错误处理和验证

- 无法满足请求时 `heap_alloc()` 返回 `NULL`；
- 无效或重复释放不破坏堆状态；
- 物理页或块描述符耗尽时，`heap_alloc()` 返回 `NULL`，系统调用将该值传回用户；
- 不为 `better_malloc()` 和 `better_free()` 添加测试；
- 使用以下命令验证课程给定应用：

```bash
make clean && make march=-march=rv64imafd
spike --isa=rv64imafd obj/riscv-pke obj/app_singlepageheap
```

成功标准：应用输出 `hello, world!!!`，不打印地址空间管理错误，并以退出码 0 关闭系统。

## 9. 设计取舍

外置内核描述符比把块头写入用户页更容易保证用户数据边界，也能让 `better_free()` 通过用户虚拟地址精确找到块。固定单页范围牺牲了通用堆的容量，但降低了本挑战的实现复杂度，并直接满足给定应用的“同页分配、局部释放、空闲块复用”要求。
