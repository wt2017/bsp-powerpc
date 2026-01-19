# bsp-powerpc

PowerPC 8548 总线结构树形图
================================

e500 Core (CPU)
├── L1 Cache (I/D)
├── L2 Cache/SRAM
│   ├── 模式1: 512KB L2 Cache
│   ├── 模式2: 256KB L2 Cache + 256KB SRAM
│   └── 模式3: 512KB SRAM
│
├── Coherent Core Bus (CCB)
│   ├── DDR SDRAM Controller
│   │   └── DDR Memory (0x00000000-0x1FFFFFFF, 512MB)
│   │       ├── Bank 0-3
│   │       └── ECC Support (可选)
│   │
│   ├── Local Bus Controller (LBC)
│   │   ├── Chip Select 0 (CS0): NOR Flash 0
│   │   │   └── 0xFC000000-0xFFFFFFFF (64MB)
│   │   ├── Chip Select 1 (CS1): NOR Flash 1
│   │   │   └── 0xFF000000 (UBoot区域)
│   │   ├── Chip Select 2 (CS2): LBC SDRAM (可选)
│   │   │   └── 0xF0000000 (64MB)
│   │   ├── Chip Select 3 (CS3): NVRAM/CPLD (可选)
│   │   │   └── 0xA0000000 (16MB)
│   │   ├── Chip Select 4 (CS4): 保留
│   │   ├── Chip Select 5 (CS5): 保留
│   │   ├── Chip Select 6 (CS6): 保留
│   │   └── Chip Select 7 (CS7): 保留
│   │
│   ├── PCI Host Bridge 1
│   │   └── PCI Bus 1
│   │       ├── PCI Slot 1
│   │       ├── PCI Slot 2
│   │       ├── PCI Slot 3
│   │       └── PCI Slot 4
│   │
│   ├── PCI Host Bridge 2 (可选)
│   │   └── PCI Bus 2
│   │       └── PCI Slot 1
│   │
│   ├── PCI Express Root Port (可选)
│   │   └── PCIe Bus
│   │
│   ├── RapidIO Controller (可选)
│   │   └── RapidIO Port
│   │
│   ├── TSEC Ethernet Controller
│   │   ├── TSEC1: Primary Ethernet
│   │   └── TSEC3: Tertiary Ethernet
│   │
│   ├── DUART Controller
│   │   ├── UART Channel 1 (COM1)
│   │   └── UART Channel 2 (COM2)
│   │
│   ├── I2C Controller
│   │   ├── I2C1
│   │   └── I2C2
│   │
│   └── EPIC Interrupt Controller
│       ├── 外部中断输入
│       └── 内部外设中断
│
└── Local Access Windows (LAW) - 地址映射控制
    ├── LAW0: LBC CS0/CS1 → 0xF0000000 (Flash)
    ├── LAW1: DDR Controller → 0x00000000 (DDR SDRAM)
    ├── LAW2: LBC CS2 → 0xA0000000 (LBC SDRAM)
    ├── LAW3: LBC CS3 → 0xB0000000 (备用Flash)
    ├── LAW4: PCI Host 1 → 0x80000000 (PCI 1.0空间)
    ├── LAW5: PCI Host 2 → 0x58000000 (PCI 2.0空间，可选)
    ├── LAW6: PCI Express → 0x60000000 (PCIe空间，可选)
    ├── LAW7: RapidIO → 0xC0000000 (RapidIO空间，可选)
    ├── LAW8: LBC CS4-7 → 0xC0000000-0xDFFFFFFF (保留)
    └── LAW9: LBC CS4-7 → 0xD0000000-0xDFFFFFFF (保留)

地址空间映射关系
================

CPU 视角地址空间:
0x00000000-0x1FFFFFFF ────[LAW1]───→ DDR SDRAM (512MB)
0x80000000-0x8FFFFFFF ────[LAW4]───→ PCI 1.0空间 (256MB)
0xA0000000-0xAFFFFFFF ────[LAW2]───→ LBC CS3 (NVRAM/CPLD, 16MB)
0xB0000000-0xBFFFFFFF ────[LAW3]───→ LBC 备用Flash (16MB)
0xC0000000-0xCFFFFFFF ────[LAW7]───→ RapidIO (256MB，可选)
0xD0000000-0xDFFFFFFF ────[LAW9]───→ LBC 保留空间
0xE0000000-0xE00FFFFF ────────────→ CCSBAR (配置空间，1MB)
0xF0000000-0xFFFFFFFF ────[LAW0]───→ LBC CS0/CS1/CS2 (Flash/LBC SDRAM)

外设到总线的连接关系:
1. NOR Flash → LBC → CCB → CPU
2. DDR SDRAM → DDR Controller → CCB → CPU  
3. PCI设备 → PCI总线 → PCI Host Bridge → CCB → CPU
4. UART → DUART Controller → CCB → CPU
5. Ethernet → TSEC Controller → CCB → CPU
6. I2C设备 → I2C Controller → CCB → CPU

关键特点:
1. LAW提供灵活的地址映射，允许不同外设映射到不同的CPU地址空间
2. LBC作为低速外设的统一接口，支持多种存储设备
3. 多级总线结构：CPU核心总线 → CCB → 各外设控制器
4. 支持缓存一致性（Coherent Core Bus）
5. 可配置的内存和外设选项，通过config.h宏定义控制

