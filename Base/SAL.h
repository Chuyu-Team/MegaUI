#pragma once

#include <Base/YY.h>

#ifdef _PREFAST_
#define __YY_IGNORE_UNINITIALIZED_VARIABLE(_VAR) memset(_VAR, 0, 0);
#else
// ���� C6001 ���棬ʹ��δ��ʼ���ı�������
#define __YY_IGNORE_UNINITIALIZED_VARIABLE(_VAR)
#endif

// ���ú���SAL��һ�¾���
#define __YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION() __pragma(warning(disable:28251))