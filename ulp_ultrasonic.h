#pragma once
#include <Arduino.h>

// API
void ULP_Wind_init(uint32_t baud = 38400);
void ULP_Wind_warmup(uint32_t ms = 12000);
bool ULP_Wind_capture(uint32_t window_ms = 2500, float* out_mps = nullptr, int* out_deg = nullptr);
