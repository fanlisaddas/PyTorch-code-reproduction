# 简单复现 PyTorch 相关机制
通过阅读 PyTorch 源码，在源码的基础上进行复现。为方便阅读，代码中类、函数和变量的命名均和 PyTorch 保持一致。

在源码基础上，我进行了大量简化工作，包括简化了很多封装，仅考虑 CPU 设备，以及不考虑多线程情况等等，
希望能简单完整的复现出 Tensor 的相关功能，同时又能对 PyTorch 中一些经典设计予以保留。

目前已经基本复现出 Tensor 的底层存储结构 Storage 和 StorageImpl；为 Tensor 分配内存空间的接口 Allocator
和在 CPU 上按字节分配内存的分配器 DefaultCPUAllocator；以及共享指针 intrusive_ptr。

详见知乎专栏： [PyTorch源码分析与复现](https://www.zhihu.com/column/c_1669017654416502784).

做这件事的出发点是我一开始接触 PyTorch 时，希望能够更深入的了解它的底层机制，但是浏览了很多资料后
发现并没有找到令我完全满意的 PyTorch 源码解读。于是我希望利用业余时间阅读 PyTorch 源码并整理自己的笔
记。同时考虑到很多人是没有条件和机会编译 PyTorch 源码，也就无法真正意义上去尝试和探究 PyTorch 中的机制，
因此我在源码的基础上进行复现。目前只需要配置 C++ 环境就可以运行，专栏中每篇文章的结尾处给出了简单的测例。

测例：
```
#include <iostream>

#include "tensor/Allocator.hpp"
#include "tensor/alloc_cpu.hpp"
#include "tensor/Storage.hpp"
#include "tensor/CPUAllocator.hpp"

int main(void) {
    c10::DefaultCPUAllocator* allocator = new c10::DefaultCPUAllocator;
    size_t len = 1;
    c10::Storage* st = new c10::Storage(len, allocator, false);
    std::cout << st->data() << std::endl;  // 内存首地址
    std::cout << st->use_count() << std::endl;  // StorageImpl 的引用计数
    delete st;
    delete allocator;
}
```
