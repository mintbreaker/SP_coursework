#include "system_info.h"

void update_gpu_info(SystemInfo* info) {
    // Попытка получить информацию о GPU через nvidia-smi
    FILE* file = popen("nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu --format=csv,noheader,nounits", "r");
    if (file != NULL) {
        char line[256];
        if (fgets(line, sizeof(line), file)) {
            int gpu_usage, gpu_mem_used, gpu_mem_total, gpu_temp;
            if (sscanf(line, "%d, %d, %d, %d", &gpu_usage, &gpu_mem_used, &gpu_mem_total, &gpu_temp) == 4) {
                info->gpu_usage = gpu_usage;
                info->gpu_mem_used = gpu_mem_used / 1024.0; // Convert to GB
                info->gpu_mem_total = gpu_mem_total / 1024.0; // Convert to GB
                info->gpu_temp = gpu_temp;
            }
        }
        pclose(file);
    } else {
        // Если nvidia-smi недоступен, пробуем получить информацию через другие методы
        info->gpu_usage = 0;
        info->gpu_mem_used = 0;
        info->gpu_mem_total = 0;
        info->gpu_temp = 0;
    }
} 