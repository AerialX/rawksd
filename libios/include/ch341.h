#pragma once

#include "gctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

int ch341_send(const void *_data, u32 size);
int ch341_open();
void ch341_close();

#ifdef __cplusplus
}
#endif
