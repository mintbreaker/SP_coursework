#include "system_info.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>

void print_system_info(const SystemInfo* info) {
    printf("\033[2J\033[H"); // Clear screen and move cursor to top-left
    printf("System Monitor\n");
    printf("==============\n\n");
    
    // CPU Information
    printf("CPU:\n");
    printf("  Usage: %.1f%%\n", info->cpu_usage);
    printf("  Temperature: %.1f°C\n", info->cpu_temp);
    printf("  Load: %.2f %.2f %.2f (1min 5min 15min)\n\n",
           info->load_1min, info->load_5min, info->load_15min);
    
    // Memory Information
    printf("Memory:\n");
    printf("  Total: %.1f GB\n", info->memory_total);
    printf("  Used:  %.1f GB\n", info->memory_used);
    printf("  Free:  %.1f GB\n", info->memory_free);
    printf("  Cached: %.1f GB\n\n", info->memory_cached);
    
    // Disk Information
    printf("Disk:\n");
    printf("  Total: %.1f GB\n", info->disk_total);
    printf("  Used:  %.1f GB\n", info->disk_used);
    printf("  Free:  %.1f GB\n\n", info->disk_free);
    
    // GPU Information
    if (info->gpu_usage > 0) {
        printf("GPU:\n");
        printf("  Usage: %d%%\n", info->gpu_usage);
        printf("  Memory: %.1f/%.1f GB\n", info->gpu_mem_used, info->gpu_mem_total);
        printf("  Temperature: %.1f°C\n\n", info->gpu_temp);
    }
    
    // Network Information
    printf("Network:\n");
    printf("  RX: %.2f MB\n", info->network_rx / (1024.0 * 1024.0));
    printf("  TX: %.2f MB\n", info->network_tx / (1024.0 * 1024.0));
    
    fflush(stdout);
}

int main() {
    SystemInfo info;
    time_t last_network_update = 0;
    unsigned long last_rx = 0, last_tx = 0;
    
    while (1) {
        // Обновляем информацию о системе
        update_cpu_info(&info);
        update_memory_info(&info);
        update_disk_info(&info);
        update_gpu_info(&info);
        
        // Обновляем сетевую статистику каждую секунду
        time_t now = time(NULL);
        if (now - last_network_update >= 1) {
            update_network_info(&info);
            last_network_update = now;
        }
        
        // Выводим информацию
        print_system_info(&info);
        
        // Ждем 1 секунду
        sleep(1);
    }
    
    return 0;
} 