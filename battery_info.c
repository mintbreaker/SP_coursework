#include "system_info.h"

void update_battery_info(SystemInfo* info) {
    FILE* file = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (file != NULL) {
        if (fscanf(file, "%d", &info->battery_capacity) != 1) {
            info->battery_capacity = 0;
        }
        fclose(file);
    } else {
        info->battery_capacity = 0;
    }

    file = fopen("/sys/class/power_supply/BAT0/status", "r");
    if (file != NULL) {
        if (fgets(info->battery_status, sizeof(info->battery_status), file) != NULL) {
            // Удаляем символ новой строки
            info->battery_status[strcspn(info->battery_status, "\n")] = 0;
        } else {
            strcpy(info->battery_status, "Unknown");
        }
        fclose(file);
    } else {
        strcpy(info->battery_status, "Unknown");
    }
} 