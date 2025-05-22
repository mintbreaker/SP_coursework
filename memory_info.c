#include "system_info.h"

void update_memory_info(SystemInfo* info) {
    FILE* file = fopen("/proc/meminfo", "r");
    if (file != NULL) {
        char line[256];
        unsigned long total = 0, free = 0, buffers = 0, cached = 0;
        
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                sscanf(line, "MemTotal: %lu", &total);
            } else if (strncmp(line, "MemFree:", 8) == 0) {
                sscanf(line, "MemFree: %lu", &free);
            } else if (strncmp(line, "Buffers:", 8) == 0) {
                sscanf(line, "Buffers: %lu", &buffers);
            } else if (strncmp(line, "Cached:", 7) == 0) {
                sscanf(line, "Cached: %lu", &cached);
            }
        }
        fclose(file);
        
        // Конвертируем в гигабайты
        info->memory_total = total / (1024.0 * 1024.0);
        info->memory_free = free / (1024.0 * 1024.0);
        info->memory_used = info->memory_total - info->memory_free;
        info->memory_cached = (buffers + cached) / (1024.0 * 1024.0);
    } else {
        info->memory_total = 0;
        info->memory_free = 0;
        info->memory_used = 0;
        info->memory_cached = 0;
    }
} 