#ifndef __GPIO_H__
#define __GPIO_H__

#include <gctypes.h>

#define GPIO_BASE 			0xd800000
#define GPIO_OUT			0x0E0
#define GPIO_DIR			0x0E4
#define GPIO_IN 			0x0E8
#define GPIO_IS_PPC			0x0FC

#define GPIO_OSPIN 			0x10
#define GPIO_OSLOT 			0x20
#define GPIO_IDISC 			0x80
#define GPIO_OEJECT 		0x200

#ifdef __cplusplus
extern "C" {
#endif

void gpio_set_on(u32 flag);
void gpio_set_off(u32 flag);
void gpio_set_toggle(u32 flag);
bool gpio_get(u32 flag);
void gpio_enable_toggle(u32 flag);
void gpio_disable_toggle(u32 flag);

#ifdef __cplusplus
}
#endif

#endif
