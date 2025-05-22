#include "system_info.h"
#include <sys/statvfs.h>

void update_disk_info(SystemInfo* info) {
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        // Расчет общего и свободного места в гигабайтах
        unsigned long long total_bytes = stat.f_blocks * stat.f_frsize;
        unsigned long long free_bytes = stat.f_bfree * stat.f_frsize;
        
        info->disk_total = total_bytes / (1024.0 * 1024.0 * 1024.0); // Convert to GB
        info->disk_free = free_bytes / (1024.0 * 1024.0 * 1024.0);   // Convert to GB
        info->disk_used = info->disk_total - info->disk_free;
    } else {
        info->disk_total = 0;
        info->disk_free = 0;
        info->disk_used = 0;
    }
} 