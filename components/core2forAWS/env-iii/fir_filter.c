#include "core2forAWS.h"
#include "fir_filter.h"

fir_filter_t * init_fir_filter(uint32_t depth, double initial_value) {
    double * buffer = malloc(sizeof(double) * depth);
    if (buffer == NULL) {
        return NULL;
    }

    fir_filter_t * filter = malloc(sizeof(fir_filter_t));
    if (filter == NULL) {
        free(buffer);
        return NULL;
    }

    for (int i=0; i<depth; i++) {
        buffer[i] = initial_value;
    }

    filter->depth = depth;
    filter->average = initial_value;
    filter->sum = initial_value * depth;
    filter->sequence = buffer;
    filter->index = 0;

    return filter;
}

fir_filter_t * reinit_fir_filter(fir_filter_t * filter, uint32_t depth, double initial_value) {
    if (depth > filter->depth) {
        double * buffer = malloc(sizeof(double) * depth);

        if (buffer == NULL) {
            return NULL;
        }

        for (int i=0; i<depth; i++) {
            buffer[i] = initial_value;
        }

        filter->sequence = buffer;
    }

    filter->depth = depth;

    return filter;
}

void deinit_fir_filter(fir_filter_t * filter) {
    if (filter != NULL) {
        if (filter->sequence != NULL) {
            free(filter->sequence);
        }
        free(filter);
    }
}

double fir_filter_process(fir_filter_t * filter, double data) {
    double last = filter->sequence[filter->index];

    filter->sequence[filter->index] = data;

    filter->index += 1;
    if (filter->index >= filter->depth) {
        filter->index = 0;
    }

    filter->sum += (data - last);
    filter->average = filter->sum / filter->depth;

    return filter->average;
}
