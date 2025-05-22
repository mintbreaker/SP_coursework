#include "system_monitor.h"

void update_system_info(SystemInfo* sys_info) {
    // Инициализация значений по умолчанию
    memset(sys_info, 0, sizeof(SystemInfo));
    
    // CPU Cores and Usage
    FILE* file = fopen("/proc/cpuinfo", "r");
    if (file != NULL) {
        char line[256];
        sys_info->cpu_cores = 0;
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "processor", 9) == 0) {
                sys_info->cpu_cores++;
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
            sys_info->cpu_usage = 100.0 * (1.0 - ((float)idle / total));
        }
        
        // Читаем статистику для каждого ядра
        for (int i = 0; i < sys_info->cpu_cores; i++) {
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
                        sys_info->cpu_per_core[i] = 100.0 * (1.0 - ((float)idle / total));
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
            sys_info->cpu_temp = temp / 1000.0;
        }
        fclose(file);
    }

    // Memory Usage
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        sys_info->mem_total = (float)si.totalram / (1024 * 1024 * 1024);
        sys_info->mem_used = (float)(si.totalram - si.freeram) / (1024 * 1024 * 1024);
        sys_info->mem_usage = (sys_info->mem_used / sys_info->mem_total) * 100;
        
        // Get detailed memory info from /proc/meminfo
        file = fopen("/proc/meminfo", "r");
        if (file != NULL) {
            char line[256];
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "Buffers:") != NULL) {
                    sscanf(line, "Buffers: %f", &sys_info->mem_buffers);
                    sys_info->mem_buffers /= 1024 * 1024; // Convert to GB
                }
                if (strstr(line, "Cached:") != NULL) {
                    sscanf(line, "Cached: %f", &sys_info->mem_cached);
                    sys_info->mem_cached /= 1024 * 1024; // Convert to GB
                }
            }
            fclose(file);
        }
        
        sys_info->system_load[0] = si.loads[0] / 65536.0;
        sys_info->system_load[1] = si.loads[1] / 65536.0;
        sys_info->system_load[2] = si.loads[2] / 65536.0;
        
        sys_info->uptime = si.uptime;
    }

    // Top Processes with command names
    file = popen("ps -eo pid,%cpu,%mem,comm --sort=-%cpu | head -n 6", "r");
    if (file != NULL) {
        char line[256];
        int i = 0;
        // Пропускаем заголовок
        fgets(line, sizeof(line), file);
        while (fgets(line, sizeof(line), file) && i < MAX_PROCESSES) {
            int pid;
            float cpu, mem;
            char comm[256];
            if (sscanf(line, "%d %f %f %s", &pid, &cpu, &mem, comm) == 4) {
                sys_info->process_pid[i] = pid;
                sys_info->process_cpu[i] = cpu;
                sys_info->process_mem[i] = mem;
                strncpy(sys_info->process_name[i], comm, sizeof(sys_info->process_name[i]) - 1);
                sys_info->process_name[i][sizeof(sys_info->process_name[i]) - 1] = '\0';
                i++;
            }
        }
        pclose(file);
    }

    // Network Interfaces
    sys_info->net_interface_count = 0;
    file = fopen("/proc/net/dev", "r");
    if (file != NULL) {
        char line[256];
        // Skip header lines
        fgets(line, sizeof(line), file);
        fgets(line, sizeof(line), file);
        
        while (fgets(line, sizeof(line), file) && sys_info->net_interface_count < MAX_NET_INTERFACES) {
            char name[32];
            unsigned long long rx_bytes, tx_bytes, rx_packets, tx_packets;
            
            if (sscanf(line, "%[^:]: %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu",
                      name, &rx_bytes, &rx_packets, &tx_bytes, &tx_packets) == 5) {
                strncpy(sys_info->net_interfaces[sys_info->net_interface_count].name, name,
                       sizeof(sys_info->net_interfaces[sys_info->net_interface_count].name) - 1);
                sys_info->net_interfaces[sys_info->net_interface_count].name[sizeof(sys_info->net_interfaces[sys_info->net_interface_count].name) - 1] = '\0';
                sys_info->net_interfaces[sys_info->net_interface_count].rx_bytes = rx_bytes;
                sys_info->net_interfaces[sys_info->net_interface_count].tx_bytes = tx_bytes;
                sys_info->net_interfaces[sys_info->net_interface_count].rx_packets = rx_packets;
                sys_info->net_interfaces[sys_info->net_interface_count].tx_packets = tx_packets;
                sys_info->net_interface_count++;
            }
        }
        fclose(file);
    }

    // Disk Usage
    sys_info->disk_count = 0;
    file = popen("df -h | grep '^/dev/'", "r");
    if (file != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), file) && sys_info->disk_count < MAX_DISKS) {
            char name[32], size[32], used[32], avail[32], use[32], mount[32];
            if (sscanf(line, "%s %s %s %s %s %s", name, size, used, avail, use, mount) == 6) {
                strncpy(sys_info->disks[sys_info->disk_count].name, name,
                       sizeof(sys_info->disks[sys_info->disk_count].name) - 1);
                sys_info->disks[sys_info->disk_count].name[sizeof(sys_info->disks[sys_info->disk_count].name) - 1] = '\0';
                sys_info->disks[sys_info->disk_count].total = atof(size);
                sys_info->disks[sys_info->disk_count].used = atof(used);
                sys_info->disks[sys_info->disk_count].free = atof(avail);
                sys_info->disks[sys_info->disk_count].usage = atof(use);
                sys_info->disk_count++;
            }
        }
        pclose(file);
    }

    // Battery Info
    file = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (file != NULL) {
        fscanf(file, "%d", &sys_info->battery_capacity);
        fclose(file);
        
        file = fopen("/sys/class/power_supply/BAT0/status", "r");
        if (file != NULL) {
            if (fgets(sys_info->battery_status, sizeof(sys_info->battery_status), file)) {
                sys_info->battery_status[strcspn(sys_info->battery_status, "\n")] = 0;
            }
            fclose(file);
        }
    }

    // GPU Info
    if (system("which nvidia-smi >/dev/null 2>&1") == 0) {
        file = popen("nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu --format=csv,noheader,nounits", "r");
        if (file != NULL) {
            int gpu_util, mem_used, mem_total, temp;
            if (fscanf(file, "%d, %d, %d, %d", &gpu_util, &mem_used, &mem_total, &temp) == 4) {
                sys_info->gpu_usage = gpu_util;
                sys_info->gpu_mem_used = mem_used;
                sys_info->gpu_mem_total = mem_total;
                sys_info->gpu_temp = temp;
            }
            pclose(file);
        }
    }
} 