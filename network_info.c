#include "system_info.h"
#include <string.h>

void update_network_info(SystemInfo* info) {
    // Получение информации о сетевых интерфейсах
    FILE* file = fopen("/proc/net/dev", "r");
    if (file != NULL) {
        char line[256];
        // Пропускаем заголовки
        fgets(line, sizeof(line), file);
        fgets(line, sizeof(line), file);
        
        // Читаем информацию о каждом интерфейсе
        while (fgets(line, sizeof(line), file)) {
            char iface[32];
            unsigned long rx_bytes, tx_bytes;
            
            if (sscanf(line, "%31[^:]: %lu %*u %*u %*u %*u %*u %*u %*u %lu",
                      iface, &rx_bytes, &tx_bytes) == 3) {
                // Пропускаем локальные интерфейсы
                if (strncmp(iface, "lo", 2) == 0) continue;
                
                // Обновляем информацию о сетевом трафике
                info->network_rx = rx_bytes;
                info->network_tx = tx_bytes;
                break; // Берем только первый нелокальный интерфейс
            }
        }
        fclose(file);
    }
} 