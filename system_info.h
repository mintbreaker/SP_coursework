#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

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
    // CPU информация
    double cpu_usage;
    double cpu_temp;
    float load_1min;
    float load_5min;
    float load_15min;
    
    // Память
    double memory_total;
    double memory_used;
    double memory_free;
    double memory_cached;
    
    // Диск
    double disk_total;
    double disk_used;
    double disk_free;
    
    // GPU информация
    int gpu_usage;
    double gpu_mem_used;
    double gpu_mem_total;
    double gpu_temp;
    
    // Сеть
    unsigned long network_rx;
    unsigned long network_tx;
    
    // CPU Information
    int cpu_cores;
    float cpu_per_core[MAX_CPU_CORES];
    
    // Memory Information
    float mem_usage;
    float mem_used;
    float mem_total;
    float mem_buffers;
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

// Объявление глобальной переменной
extern SystemInfo sys_info;

// Объявления функций
void update_cpu_info(SystemInfo* info);
void update_memory_info(SystemInfo* info);
void update_process_info(SystemInfo* info);
void update_network_info(SystemInfo* info);
void update_disk_info(SystemInfo* info);
void update_battery_info(SystemInfo* info);
void update_gpu_info(SystemInfo* info);
void update_system_info(void);

#endif // SYSTEM_INFO_H 