#pragma once

#include <Base/YY.h>

#ifdef _PREFAST_
#define __YY_IGNORE_UNINITIALIZED_VARIABLE(_VAR) memset(_VAR, 0, 0);
#else
// 忽略 C6001 警告，使用未初始化的变量警告
#define __YY_IGNORE_UNINITIALIZED_VARIABLE(_VAR)
#endif

// 禁用函数SAL不一致警告
#ifdef _MSC_VER
#define __YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION() __pragma(warning(disable:28251))
#else
#define __YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()
#endif

#ifndef _CRTALLOCATOR
#define _CRTALLOCATOR
#endif

// SAL Downlevel
#ifndef _Return_type_success_
#define _Return_type_success_(expr)
#endif

#ifndef _Success_
#define _Success_(expr)
#endif

#ifndef _In_
#define _In_
#endif

#ifndef _In_opt_
#define _In_opt_ _In_
#endif

#ifndef _In_z_
#define _In_z_ _In_
#endif

#ifndef _Inout_
#define _Inout_
#endif

#ifndef _In_opt_z_
#define _In_opt_z_ _In_opt_
#endif

#ifndef _In_reads_
#define _In_reads_(size) _In_
#endif

#ifndef _In_reads_bytes_
#define _In_reads_bytes_(size) _In_
#endif

#ifndef _In_reads_opt_
#define _In_reads_opt_(size) _In_opt_
#endif

#ifndef _Out_
#define _Out_
#endif

#ifndef _Out_opt_
#define _Out_opt_ _Out_
#endif

#ifndef _Out_writes_
#define _Out_writes_(size) _Out_
#endif

#ifndef _Outptr_
#define _Outptr_ _Out_
#endif

#ifndef _Inout_cap_
#define _Inout_cap_(size)
#endif

#ifndef _Always_
#define _Always_(annos)
#endif

#ifndef _Ret_notnull_
#define _Ret_notnull_
#endif

#ifndef _Ret_z_
#define _Ret_z_
#endif

#ifndef _Ret_maybenull_
#define _Ret_maybenull_
#endif

#ifndef _Ret_writes_maybenull_
#define _Ret_writes_maybenull_(size) _Ret_maybenull_
#endif

#ifndef _Post_readable_size_
#define _Post_readable_size_(size)
#endif

#ifndef _Field_size_
#define _Field_size_(size)
#endif

#ifndef _Field_z_
#define _Field_z_
#endif

#ifndef _Null_terminated_
#define _Null_terminated_
#endif

#ifndef _Return_type_success_
#define _Return_type_success_(expr)
#endif

#ifndef _Printf_format_string_
#define _Printf_format_string_
#endif

#ifndef _Translates_Win32_to_HRESULT_
#define _Translates_Win32_to_HRESULT_(errorCode)
#endif

#ifndef _Check_return_
#define _Check_return_
#endif

#ifndef _Pre_maybenull_
#define _Pre_maybenull_
#endif

#ifndef _Post_invalid_
#define _Post_invalid_
#endif

#ifndef _Post_writable_byte_size_
#define _Post_writable_byte_size_(size)
#endif

