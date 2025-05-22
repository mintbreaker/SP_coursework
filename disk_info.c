#include "system_info.h"

void update_disk_info(SystemInfo* info) {
    info->disk_count = 0;
    FILE* file = popen("df -h | grep '^/dev/'", "r");
    if (file != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), file) && info->disk_count < MAX_DISKS) {
            char name[32], size[32], used[32], avail[32], use[32], mount[32];
            if (sscanf(line, "%s %s %s %s %s %s", name, size, used, avail, use, mount) == 6) {
                strncpy(info->disks[info->disk_count].name, name,
                       sizeof(info->disks[info->disk_count].name) - 1);
                info->disks[info->disk_count].name[sizeof(info->disks[info->disk_count].name) - 1] = '\0';
                
                // Преобразуем строковые значения в числа
                sscanf(size, "%f", &info->disks[info->disk_count].total);
                sscanf(used, "%f", &info->disks[info->disk_count].used);
                sscanf(avail, "%f", &info->disks[info->disk_count].free);
                sscanf(use, "%f%%", &info->disks[info->disk_count].usage);
                
                info->disk_count++;
            }
        }
        pclose(file);
    }
} 