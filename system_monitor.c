#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <locale.h>
#include <time.h>
#include <ncurses.h>

#define MAX_CPU_STATS 7
#define REFRESH_RATE 1 // seconds
#define MAX_PROCESSES 5
#define MAX_CPU_CORES 32
#define MAX_NET_INTERFACES 8
#define MAX_DISKS 8

// Структура для хранения данных о системе
typedef struct {
    // CPU Information
    float cpu_usage;
    float cpu_temp;
    int cpu_cores;
    float cpu_per_core[MAX_CPU_CORES];
    
    // Memory Information
    float mem_usage;
    float mem_used;
    float mem_total;
    float mem_buffers;
    float mem_cached;
    float swap_usage;
    float swap_used;
    float swap_total;
    
    // System Load
    float system_load[3];
    
    // Process Information
    int process_pid[MAX_PROCESSES];
    float process_cpu[MAX_PROCESSES];
    float process_mem[MAX_PROCESSES];
    char process_name[MAX_PROCESSES][256];
    
    // Battery Information
    int battery_capacity;
    char battery_status[32];
    
    // GPU Information
    float gpu_usage;
    float gpu_temp;
    float gpu_mem_used;
    float gpu_mem_total;
    
    // Network Information
    struct {
        char name[32];
        unsigned long long rx_bytes;
        unsigned long long tx_bytes;
        unsigned long long rx_packets;
        unsigned long long tx_packets;
    } net_interfaces[MAX_NET_INTERFACES];
    int net_interface_count;
    
    // Disk Information
    struct {
        char name[32];
        float total;
        float used;
        float free;
        float usage;
    } disks[MAX_DISKS];
    int disk_count;
    
    // System Uptime
    unsigned long uptime;
} SystemInfo;

SystemInfo sys_info;

void update_system_info() {
    // Инициализация значений по умолчанию
    memset(&sys_info, 0, sizeof(SystemInfo));
    
    // CPU Cores and Usage
    FILE* file = fopen("/proc/cpuinfo", "r");
    if (file != NULL) {
        char line[256];
        sys_info.cpu_cores = 0;
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "processor", 9) == 0) {
                sys_info.cpu_cores++;
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
            sys_info.cpu_usage = 100.0 * (1.0 - ((float)idle / total));
        }
        
        // Читаем статистику для каждого ядра
        for (int i = 0; i < sys_info.cpu_cores; i++) {
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
                        sys_info.cpu_per_core[i] = 100.0 * (1.0 - ((float)idle / total));
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
            sys_info.cpu_temp = temp / 1000.0;
        }
        fclose(file);
    }

    // Memory Usage
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        sys_info.mem_total = (float)si.totalram / (1024 * 1024 * 1024);
        sys_info.mem_used = (float)(si.totalram - si.freeram) / (1024 * 1024 * 1024);
        sys_info.mem_usage = (sys_info.mem_used / sys_info.mem_total) * 100;
        
        // Get detailed memory info from /proc/meminfo
        file = fopen("/proc/meminfo", "r");
        if (file != NULL) {
            char line[256];
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "Buffers:") != NULL) {
                    sscanf(line, "Buffers: %f", &sys_info.mem_buffers);
                    sys_info.mem_buffers /= 1024 * 1024; // Convert to GB
                }
                if (strstr(line, "Cached:") != NULL) {
                    sscanf(line, "Cached: %f", &sys_info.mem_cached);
                    sys_info.mem_cached /= 1024 * 1024; // Convert to GB
                }
            }
            fclose(file);
        }
        
        sys_info.swap_total = (float)si.totalswap / (1024 * 1024 * 1024);
        sys_info.swap_used = (float)(si.totalswap - si.freeswap) / (1024 * 1024 * 1024);
        sys_info.swap_usage = sys_info.swap_total > 0 ? (sys_info.swap_used / sys_info.swap_total) * 100 : 0;
        
        sys_info.system_load[0] = si.loads[0] / 65536.0;
        sys_info.system_load[1] = si.loads[1] / 65536.0;
        sys_info.system_load[2] = si.loads[2] / 65536.0;
        
        sys_info.uptime = si.uptime;
    }

    // Top Processes with command names
    file = popen("ps -eo pid,%cpu,%mem,comm --sort=-%cpu | head -n 6", "r");
    if (file != NULL) {
        char line[256];
        int i = 0;
        while (fgets(line, sizeof(line), file) && i < MAX_PROCESSES) {
            if (strstr(line, "PID") != NULL) continue;
            int pid;
            float cpu, mem;
            char comm[256];
            if (sscanf(line, "%d %f %f %s", &pid, &cpu, &mem, comm) == 4) {
                sys_info.process_pid[i] = pid;
                sys_info.process_cpu[i] = cpu;
                sys_info.process_mem[i] = mem;
                strncpy(sys_info.process_name[i], comm, sizeof(sys_info.process_name[i]) - 1);
                sys_info.process_name[i][sizeof(sys_info.process_name[i]) - 1] = '\0';
                i++;
            }
        }
        pclose(file);
    }

    // Network Interfaces
    sys_info.net_interface_count = 0;
    file = fopen("/proc/net/dev", "r");
    if (file != NULL) {
        char line[256];
        // Skip header lines
        fgets(line, sizeof(line), file);
        fgets(line, sizeof(line), file);
        
        while (fgets(line, sizeof(line), file) && sys_info.net_interface_count < MAX_NET_INTERFACES) {
            char name[32];
            unsigned long long rx_bytes, tx_bytes, rx_packets, tx_packets;
            
            if (sscanf(line, "%[^:]: %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu",
                      name, &rx_bytes, &rx_packets, &tx_bytes, &tx_packets) == 5) {
                strncpy(sys_info.net_interfaces[sys_info.net_interface_count].name, name,
                       sizeof(sys_info.net_interfaces[sys_info.net_interface_count].name) - 1);
                sys_info.net_interfaces[sys_info.net_interface_count].name[sizeof(sys_info.net_interfaces[sys_info.net_interface_count].name) - 1] = '\0';
                sys_info.net_interfaces[sys_info.net_interface_count].rx_bytes = rx_bytes;
                sys_info.net_interfaces[sys_info.net_interface_count].tx_bytes = tx_bytes;
                sys_info.net_interfaces[sys_info.net_interface_count].rx_packets = rx_packets;
                sys_info.net_interfaces[sys_info.net_interface_count].tx_packets = tx_packets;
                sys_info.net_interface_count++;
            }
        }
        fclose(file);
    }

    // Disk Usage
    sys_info.disk_count = 0;
    file = popen("df -h | grep '^/dev/'", "r");
    if (file != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), file) && sys_info.disk_count < MAX_DISKS) {
            char name[32], size[32], used[32], avail[32], use[32], mount[32];
            if (sscanf(line, "%s %s %s %s %s %s", name, size, used, avail, use, mount) == 6) {
                strncpy(sys_info.disks[sys_info.disk_count].name, name,
                       sizeof(sys_info.disks[sys_info.disk_count].name) - 1);
                sys_info.disks[sys_info.disk_count].name[sizeof(sys_info.disks[sys_info.disk_count].name) - 1] = '\0';
                sys_info.disks[sys_info.disk_count].total = atof(size);
                sys_info.disks[sys_info.disk_count].used = atof(used);
                sys_info.disks[sys_info.disk_count].free = atof(avail);
                sys_info.disks[sys_info.disk_count].usage = atof(use);
                sys_info.disk_count++;
            }
        }
        pclose(file);
    }

    // Battery Info
    file = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (file != NULL) {
        fscanf(file, "%d", &sys_info.battery_capacity);
        fclose(file);
        
        file = fopen("/sys/class/power_supply/BAT0/status", "r");
        if (file != NULL) {
            if (fgets(sys_info.battery_status, sizeof(sys_info.battery_status), file)) {
                sys_info.battery_status[strcspn(sys_info.battery_status, "\n")] = 0;
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
                sys_info.gpu_usage = gpu_util;
                sys_info.gpu_mem_used = mem_used;
                sys_info.gpu_mem_total = mem_total;
                sys_info.gpu_temp = temp;
            }
            pclose(file);
        }
    }
}

void init_screen() {
    initscr();           // Инициализация ncurses
    start_color();       // Включаем поддержку цветов
    use_default_colors(); // Используем цвета терминала по умолчанию
    
    // Инициализация цветовых пар
    init_pair(1, COLOR_GREEN, -1);   // Для заголовков
    init_pair(2, COLOR_CYAN, -1);    // Для значений
    init_pair(3, COLOR_YELLOW, -1);  // Для предупреждений
    init_pair(4, COLOR_RED, -1);     // Для критических значений
    
    curs_set(0);         // Скрываем курсор
    cbreak();            // Отключаем буферизацию ввода
    noecho();            // Отключаем эхо ввода
    keypad(stdscr, TRUE); // Включаем поддержку функциональных клавиш
    nodelay(stdscr, TRUE); // Делаем getch() неблокирующим
}

void print_system_info() {
    int row = 0;
    int col = 0;
    
    // System Uptime
    attron(COLOR_PAIR(1));
    mvprintw(row++, col, "System Uptime: ");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    unsigned long days = sys_info.uptime / (24 * 3600);
    unsigned long hours = (sys_info.uptime % (24 * 3600)) / 3600;
    unsigned long minutes = (sys_info.uptime % 3600) / 60;
    printw("%lu days, %lu hours, %lu minutes\n", days, hours, minutes);
    attroff(COLOR_PAIR(2));
    row++;
    
    // CPU Info
    attron(COLOR_PAIR(1));
    mvprintw(row++, col, "CPU Information:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    mvprintw(row++, col, "Total Usage: %.1f%% Temperature: %.1f°C", 
             sys_info.cpu_usage, sys_info.cpu_temp);
    attroff(COLOR_PAIR(2));
    
    attron(COLOR_PAIR(1));
    mvprintw(row++, col, "Per-Core Usage:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    for (int i = 0; i < sys_info.cpu_cores; i++) {
        mvprintw(row, col + (i % 4) * 20, "Core %d: %6.1f%%", i, sys_info.cpu_per_core[i]);
        if ((i + 1) % 4 == 0) row++;
    }
    if (sys_info.cpu_cores % 4 != 0) row++;
    attroff(COLOR_PAIR(2));
    row++;
    
    // Memory Info
    attron(COLOR_PAIR(1));
    mvprintw(row++, col, "Memory Information:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    mvprintw(row++, col, "RAM: %.1f%% (%.1f/%.1f GB) [Buffers: %.1f GB, Cached: %.1f GB]",
             sys_info.mem_usage, sys_info.mem_used, sys_info.mem_total,
             sys_info.mem_buffers, sys_info.mem_cached);
    attroff(COLOR_PAIR(2));
    row++;
    
    // System Load
    attron(COLOR_PAIR(1));
    mvprintw(row++, col, "System Load: ");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    printw("%.2f %.2f %.2f (1min, 5min, 15min)",
           sys_info.system_load[0], sys_info.system_load[1], sys_info.system_load[2]);
    attroff(COLOR_PAIR(2));
    row += 2;
    
    // Disk Information
    attron(COLOR_PAIR(1));
    mvprintw(row++, col, "Disk Usage:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    for (int i = 0; i < sys_info.disk_count; i++) {
        mvprintw(row++, col, "%s: %.1f%% (%.1f/%.1f GB)",
                 sys_info.disks[i].name,
                 sys_info.disks[i].usage,
                 sys_info.disks[i].used,
                 sys_info.disks[i].total);
    }
    attroff(COLOR_PAIR(2));
    row++;
    
    // Network Information
    attron(COLOR_PAIR(1));
    mvprintw(row++, col, "Network Interfaces:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    for (int i = 0; i < sys_info.net_interface_count; i++) {
        // Пропускаем интерфейс lo (localhost)
        if (strcmp(sys_info.net_interfaces[i].name, "lo") == 0) continue;
        
        // Конвертируем байты в более читаемый формат
        float rx_mb = sys_info.net_interfaces[i].rx_bytes / (1024.0 * 1024.0);
        float tx_mb = sys_info.net_interfaces[i].tx_bytes / (1024.0 * 1024.0);
        
        // Определяем единицы измерения
        const char* rx_unit = "MB";
        const char* tx_unit = "MB";
        
        if (rx_mb > 1024) {
            rx_mb /= 1024.0;
            rx_unit = "GB";
        }
        if (tx_mb > 1024) {
            tx_mb /= 1024.0;
            tx_unit = "GB";
        }
        
        mvprintw(row, col, "%s:", sys_info.net_interfaces[i].name);
        mvprintw(row, col + 20, "Received: %.2f %s", rx_mb, rx_unit);
        mvprintw(row, col + 45, "Sent: %.2f %s", tx_mb, tx_unit);
        row++;
    }
    attroff(COLOR_PAIR(2));
    row++;
    
    // Top Processes
    attron(COLOR_PAIR(1));
    mvprintw(row++, col, "Top Processes:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    mvprintw(row++, col, "%-6s %6s %6s  %s", "PID", "CPU%", "MEM%", "COMMAND");
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if (sys_info.process_pid[i] > 0) {
            mvprintw(row++, col, "%-6d %6.1f %6.1f  %s",
                     sys_info.process_pid[i],
                     sys_info.process_cpu[i],
                     sys_info.process_mem[i],
                     sys_info.process_name[i]);
        }
    }
    attroff(COLOR_PAIR(2));
    row++;
    
    // Battery Info
    if(sys_info.battery_capacity > 0) {
        attron(COLOR_PAIR(1));
        mvprintw(row++, col, "Battery Status:");
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(2));
        mvprintw(row++, col, "Charge: %d%%", sys_info.battery_capacity);
        mvprintw(row++, col, "Status: %s", sys_info.battery_status);
        attroff(COLOR_PAIR(2));
        row++;
    }
    
    // GPU Info
    if(sys_info.gpu_usage > 0) {
        attron(COLOR_PAIR(1));
        mvprintw(row++, col, "GPU: ");
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(2));
        printw("%.1f%% %.1f/%.1f MB %.1f°C",
               sys_info.gpu_usage,
               sys_info.gpu_mem_used,
               sys_info.gpu_mem_total,
               sys_info.gpu_temp);
        attroff(COLOR_PAIR(2));
    }
    
    // Инструкция по выходу
    attron(COLOR_PAIR(3));
    mvprintw(row + 2, col, "Press 'q' to exit");
    attroff(COLOR_PAIR(3));
    
    refresh(); // Обновляем экран
}

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    init_screen();
    
    while (1) {
        update_system_info();
        print_system_info();
        
        // Проверяем нажатие клавиши 'q' для выхода
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
        
        sleep(REFRESH_RATE);
    }
    
    endwin(); // Завершаем работу с ncurses
    return 0;
} 