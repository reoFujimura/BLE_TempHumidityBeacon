#pragma once

#include <stdint.h>

typedef void(SHT31_CALLBACK)(int16_t temperature, int16_t humidity);

void SHT31_Init(void);
void SHT31_GetValue(SHT31_CALLBACK* pCallback);
