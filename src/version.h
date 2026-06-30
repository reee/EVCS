#pragma once

// 版本号与构建信息（纯 ASCII，C++ 与 RC 资源编译器共用）。
// 本文件只放可机器解析的版本/日期，不放本地化显示文本——RC 预处理器
// 在 code_page 生效前无法消化中文 UTF-8 字面量。中文显示名请直接在
// C++ 代码中用宽字符串字面量定义。

// 版本信息宏定义
#define EVCS_VERSION_MAJOR    1
#define EVCS_VERSION_MINOR    3
#define EVCS_VERSION_PATCH    1
#define EVCS_VERSION_BUILD    0

// 版本字符串（ASCII）
#define EVCS_VERSION_STRING   "1.3.1"
#define EVCS_VERSION_FULL     "1.3.1.0"

// 构建信息（ASCII，RC 安全）
#define EVCS_BUILD_DATE       "2026-06"
#define EVCS_BUILD_YEAR       "2026"

// 产品标识（ASCII，用于 RC 资源与 C++ 通用）
#define EVCS_PRODUCT_NAME_EN  "Examination Voice Command System"
#define EVCS_COMPANY_NAME     "MangQi Tech"
#define EVCS_COPYRIGHT        "Copyright (C) 2026 MangQi Tech"
