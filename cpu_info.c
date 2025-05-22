#include "system_info.h"
#include <string.h>

void update_cpu_info(SystemInfo* info) {
    FILE* file = fopen("/proc/stat", "r");
    if (file != NULL) {
        char line[256];
        if (fgets(line, sizeof(line), file)) {
            unsigned long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
            if (sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                      &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice) == 10) {
                unsigned long total = user + nice + system + idle + iowait + irq + softirq + steal;
                info->cpu_usage = 100.0 * (1.0 - ((double)idle / total));
            }
        }
        fclose(file);
    }
    
    // Получение температуры CPU
    file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (file != NULL) {
        int temp;
        if (fscanf(file, "%d", &temp) == 1) {
            info->cpu_temp = temp / 1000.0; // Convert from millidegrees to degrees
        }
        fclose(file);
    }
    
    // Получение информации о загрузке системы
    file = fopen("/proc/loadavg", "r");
    if (file != NULL) {
        if (fscanf(file, "%f %f %f", &info->load_1min, &info->load_5min, &info->load_15min) != 3) {
            info->load_1min = 0;
            info->load_5min = 0;
            info->load_15min = 0;
        }
        fclose(file);
    }
} 