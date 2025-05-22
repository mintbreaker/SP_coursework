#include "system_info.h"

void update_network_info(SystemInfo* info) {
    info->net_interface_count = 0;
    FILE* file = fopen("/proc/net/dev", "r");
    if (file != NULL) {
        char line[256];
        // Skip header lines
        fgets(line, sizeof(line), file);
        fgets(line, sizeof(line), file);
        
        while (fgets(line, sizeof(line), file) && info->net_interface_count < MAX_NET_INTERFACES) {
            char name[32];
            unsigned long long rx_bytes, tx_bytes, rx_packets, tx_packets;
            
            if (sscanf(line, "%[^:]: %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu",
                      name, &rx_bytes, &rx_packets, &tx_bytes, &tx_packets) == 5) {
                strncpy(info->net_interfaces[info->net_interface_count].name, name,
                       sizeof(info->net_interfaces[info->net_interface_count].name) - 1);
                info->net_interfaces[info->net_interface_count].name[sizeof(info->net_interfaces[info->net_interface_count].name) - 1] = '\0';
                info->net_interfaces[info->net_interface_count].rx_bytes = rx_bytes;
                info->net_interfaces[info->net_interface_count].tx_bytes = tx_bytes;
                info->net_interfaces[info->net_interface_count].rx_packets = rx_packets;
                info->net_interfaces[info->net_interface_count].tx_packets = tx_packets;
                info->net_interface_count++;
            }
        }
        fclose(file);
    }
} 