#include "system_info.h"

void update_cpu_info(SystemInfo* info) {
    // CPU Cores and Usage
    FILE* file = fopen("/proc/cpuinfo", "r");
    if (file != NULL) {
        char line[256];
        info->cpu_cores = 0;
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "processor", 9) == 0) {
                info->cpu_cores++;
            }
        }
        fclose(file);
    }

    // CPU Usage per core
    file = fopen("/proc/stat", "r");
    if (file != NULL) {
        char line[256];
        unsigned long long cpu_stats[MAX_CPU_STATS];
        
        // Читаем общую статистику CPU
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu",
                   &cpu_stats[0], &cpu_stats[1], &cpu_stats[2], &cpu_stats[3],
                   &cpu_stats[4], &cpu_stats[5], &cpu_stats[6]);
            
            unsigned long long total = 0;
            for (int i = 0; i < MAX_CPU_STATS; i++) {
                total += cpu_stats[i];
            }
            unsigned long long idle = cpu_stats[3];
            info->cpu_usage = 100.0 * (1.0 - ((float)idle / total));
        }
        
        // Читаем статистику для каждого ядра
        for (int i = 0; i < info->cpu_cores; i++) {
            if (fgets(line, sizeof(line), file)) {
                int core_num;
                if (sscanf(line, "cpu%d %llu %llu %llu %llu %llu %llu %llu",
                          &core_num, &cpu_stats[0], &cpu_stats[1], &cpu_stats[2], &cpu_stats[3],
                          &cpu_stats[4], &cpu_stats[5], &cpu_stats[6]) == 8) {
                    unsigned long long total = 0;
                    for (int j = 0; j < MAX_CPU_STATS; j++) {
                        total += cpu_stats[j];
                    }
                    unsigned long long idle = cpu_stats[3];
                    if (total > 0) {
                        info->cpu_per_core[i] = 100.0 * (1.0 - ((float)idle / total));
                    }
                }
            }
        }
        fclose(file);
    }

    // CPU Temperature
    file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (file != NULL) {
        int temp;
        if (fscanf(file, "%d", &temp) == 1) {
            info->cpu_temp = temp / 1000.0;
        }
        fclose(file);
    }
} 