#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>

/*
 * 本工程不是完整 AUTOSAR，但保留 AUTOSAR 常见的基础类型命名。
 * 这样 BSW/RTE/APP 的函数签名更接近车载软件习惯，也方便后续继续扩展。
 */
typedef uint8_t Std_ReturnType;

#ifndef E_OK
#define E_OK        0u
#endif

#ifndef E_NOT_OK
#define E_NOT_OK    1u
#endif

#ifndef TRUE
#define TRUE        1u
#endif

#ifndef FALSE
#define FALSE       0u
#endif

#endif /* STD_TYPES_H */
