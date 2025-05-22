#include "system_monitor.h"

void init_screen() {
    initscr();
    start_color();
    use_default_colors();
    
    // Инициализация цветовых пар
    init_pair(1, COLOR_CYAN, -1);    // Заголовки
    init_pair(2, COLOR_WHITE, -1);   // Основные значения
    init_pair(3, COLOR_RED, -1);     // Предупреждения
    init_pair(4, COLOR_GREEN, -1);   // Дополнительная информация
    init_pair(5, COLOR_YELLOW, -1);  // Процессы
    init_pair(6, COLOR_MAGENTA, -1); // Сетевые интерфейсы
    
    noecho();
    curs_set(0);
    timeout(100);
    cbreak();
    keypad(stdscr, TRUE);
}

void print_system_info(const SystemInfo* sys_info) {
    move(0, 0);
    
    // Верхняя панель: Uptime и System Load
    attron(COLOR_PAIR(1));
    mvprintw(0, 0, "System Uptime: ");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    int hours = sys_info->uptime / 3600;
    int minutes = (sys_info->uptime % 3600) / 60;
    int seconds = sys_info->uptime % 60;
    printw("%02d:%02d:%02d", hours, minutes, seconds);
    attroff(COLOR_PAIR(2));
    
    attron(COLOR_PAIR(1));
    mvprintw(0, 30, "System Load: ");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    printw("%.2f %.2f %.2f", sys_info->system_load[0], sys_info->system_load[1], sys_info->system_load[2]);
    attroff(COLOR_PAIR(2));
    
    // Левая колонка: CPU и Memory
    attron(COLOR_PAIR(1));
    mvprintw(2, 0, "CPU Information:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    mvprintw(3, 2, "Total Usage: %.1f%%", sys_info->cpu_usage);
    mvprintw(4, 2, "Temperature: %.1f°C", sys_info->cpu_temp);
    attroff(COLOR_PAIR(2));
    
    attron(COLOR_PAIR(4));
    mvprintw(5, 2, "Per-core Usage:");
    attroff(COLOR_PAIR(4));
    for (int i = 0; i < sys_info->cpu_cores; i++) {
        mvprintw(6 + i, 4, "Core %d: %5.1f%%", i, sys_info->cpu_per_core[i]);
    }
    
    attron(COLOR_PAIR(1));
    mvprintw(8 + sys_info->cpu_cores, 0, "Memory Information:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    mvprintw(9 + sys_info->cpu_cores, 2, "Total: %.1f GB", sys_info->mem_total);
    mvprintw(10 + sys_info->cpu_cores, 2, "Used:  %.1f GB (%.1f%%)", 
             sys_info->mem_used, sys_info->mem_usage);
    mvprintw(11 + sys_info->cpu_cores, 2, "Buffers: %.1f GB", sys_info->mem_buffers);
    mvprintw(12 + sys_info->cpu_cores, 2, "Cached:  %.1f GB", sys_info->mem_cached);
    attroff(COLOR_PAIR(2));
    
    // Правая колонка: Disk и Network
    attron(COLOR_PAIR(1));
    mvprintw(2, 40, "Disk Usage:");
    attroff(COLOR_PAIR(1));
    
    for (int i = 0; i < sys_info->disk_count; i++) {
        attron(COLOR_PAIR(2));
        mvprintw(3 + i, 42, "%s: %.1f GB / %.1f GB (%.1f%%)",
                 sys_info->disks[i].name,
                 sys_info->disks[i].used,
                 sys_info->disks[i].total,
                 sys_info->disks[i].usage);
        attroff(COLOR_PAIR(2));
    }
    
    attron(COLOR_PAIR(1));
    mvprintw(8 + sys_info->disk_count, 40, "Network Interfaces:");
    attroff(COLOR_PAIR(1));
    
    for (int i = 0; i < sys_info->net_interface_count; i++) {
        if (strcmp(sys_info->net_interfaces[i].name, "lo") == 0) continue;
        
        attron(COLOR_PAIR(6));
        mvprintw(9 + sys_info->disk_count + i, 42, "%s:", 
                 sys_info->net_interfaces[i].name);
        attroff(COLOR_PAIR(6));
        
        attron(COLOR_PAIR(2));
        mvprintw(9 + sys_info->disk_count + i, 55, 
                 "Received: %.2f MB", sys_info->net_interfaces[i].rx_bytes / (1024.0 * 1024.0));
        mvprintw(9 + sys_info->disk_count + i, 85, 
                 "Sent: %.2f MB", sys_info->net_interfaces[i].tx_bytes / (1024.0 * 1024.0));
        attroff(COLOR_PAIR(2));
    }
    
    // Нижняя часть: Battery, GPU и Processes
    int bottom_start = 14 + (sys_info->cpu_cores > sys_info->disk_count ? sys_info->cpu_cores : sys_info->disk_count);
    
    // Battery Status
    if (sys_info->battery_capacity > 0) {
        attron(COLOR_PAIR(1));
        mvprintw(bottom_start, 0, "Battery Status:");
        attroff(COLOR_PAIR(1));
        
        attron(COLOR_PAIR(2));
        mvprintw(bottom_start + 1, 2, "Charge: %d%%", sys_info->battery_capacity);
        mvprintw(bottom_start + 2, 2, "Status: %s", sys_info->battery_status);
        attroff(COLOR_PAIR(2));
    }
    
    // GPU Information
    if (sys_info->gpu_usage > 0) {
        attron(COLOR_PAIR(1));
        mvprintw(bottom_start, 30, "GPU Information:");
        attroff(COLOR_PAIR(1));
        
        attron(COLOR_PAIR(2));
        mvprintw(bottom_start + 1, 32, "Usage: %.1f%%", sys_info->gpu_usage);
        mvprintw(bottom_start + 2, 32, "Memory: %.1f MB / %.1f MB", 
                 sys_info->gpu_mem_used, sys_info->gpu_mem_total);
        mvprintw(bottom_start + 3, 32, "Temperature: %.1f°C", sys_info->gpu_temp);
        attroff(COLOR_PAIR(2));
    }
    
    // Top Processes
    attron(COLOR_PAIR(1));
    mvprintw(bottom_start, 60, "Top Processes:");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(4));
    mvprintw(bottom_start + 1, 62, "PID     CPU%%   MEM%%   COMMAND");
    attroff(COLOR_PAIR(4));
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (sys_info->process_pid[i] > 0) {
            attron(COLOR_PAIR(5));
            mvprintw(bottom_start + 2 + i, 62, 
                     "%-6d %6.1f %6.1f   %s",
                     sys_info->process_pid[i],
                     sys_info->process_cpu[i],
                     sys_info->process_mem[i],
                     sys_info->process_name[i]);
            attroff(COLOR_PAIR(5));
        }
    }
    
    // Exit instruction
    attron(COLOR_PAIR(3));
    mvprintw(LINES - 1, 0, "Press 'q' to exit");
    attroff(COLOR_PAIR(3));
    
    doupdate();
} 