#include "system_info.h"

void update_memory_info(SystemInfo* info) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info->mem_total = (float)si.totalram / (1024 * 1024 * 1024);
        info->mem_used = (float)(si.totalram - si.freeram) / (1024 * 1024 * 1024);
        info->mem_usage = (info->mem_used / info->mem_total) * 100;
        
        // Get detailed memory info from /proc/meminfo
        FILE* file = fopen("/proc/meminfo", "r");
        if (file != NULL) {
            char line[256];
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "Buffers:") != NULL) {
                    sscanf(line, "Buffers: %f", &info->mem_buffers);
                    info->mem_buffers /= 1024 * 1024; // Convert to GB
                }
                if (strstr(line, "Cached:") != NULL) {
                    sscanf(line, "Cached: %f", &info->mem_cached);
                    info->mem_cached /= 1024 * 1024; // Convert to GB
                }
            }
            fclose(file);
        }
        
        info->swap_total = (float)si.totalswap / (1024 * 1024 * 1024);
        info->swap_used = (float)(si.totalswap - si.freeswap) / (1024 * 1024 * 1024);
        info->swap_usage = info->swap_total > 0 ? (info->swap_used / info->swap_total) * 100 : 0;
        
        info->system_load[0] = si.loads[0] / 65536.0;
        info->system_load[1] = si.loads[1] / 65536.0;
        info->system_load[2] = si.loads[2] / 65536.0;
        
        info->uptime = si.uptime;
    }
} 