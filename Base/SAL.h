#pragma once

#include <Base/YY.h>

#ifdef _PREFAST_
#define __YY_IGNORE_UNINITIALIZED_VARIABLE(_VAR) memset(_VAR, 0, 0);
#else
// 忽略 C6001 警告，使用未初始化的变量警告
#define __YY_IGNORE_UNINITIALIZED_VARIABLE(_VAR)
#endif

// 禁用函数SAL不一致警告
#define __YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION() __pragma(warning(disable:28251))