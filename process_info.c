#include "system_info.h"

void update_process_info(SystemInfo* info) {
    FILE* file = popen("ps -eo pid,%cpu,%mem,comm --sort=-%cpu | head -n 6", "r");
    if (file != NULL) {
        char line[256];
        int i = 0;
        while (fgets(line, sizeof(line), file) && i < MAX_PROCESSES) {
            if (strstr(line, "PID") != NULL) continue;
            int pid;
            float cpu, mem;
            char comm[256];
            if (sscanf(line, "%d %f %f %s", &pid, &cpu, &mem, comm) == 4) {
                info->process_pid[i] = pid;
                info->process_cpu[i] = cpu;
                info->process_mem[i] = mem;
                strncpy(info->process_name[i], comm, sizeof(info->process_name[i]) - 1);
                info->process_name[i][sizeof(info->process_name[i]) - 1] = '\0';
                i++;
            }
        }
        pclose(file);
    }
} 