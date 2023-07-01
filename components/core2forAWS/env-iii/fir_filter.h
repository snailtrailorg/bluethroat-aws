#pragma once

#include <stdint.h>

typedef struct {
    double * sequence;
    double sum;
    double average;
    uint32_t depth;
    uint32_t index;
} fir_filter_t;

fir_filter_t * init_fir_filter(uint32_t depth, double initial_value);
fir_filter_t * reinit_fir_filter(fir_filter_t * filter, uint32_t depth, double initial_value);
void deinit_fir_filter(fir_filter_t * filter);
double fir_filter_process(fir_filter_t * filter, double data);