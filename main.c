#include "system_monitor.h"

int main() {
    SystemInfo sys_info;
    
    // Инициализация экрана
    init_screen();
    
    // Основной цикл программы
    while (1) {
        // Обновление информации
        update_system_info(&sys_info);
        
        // Отображение информации
        print_system_info(&sys_info);
        
        // Проверка нажатия клавиши 'q' для выхода
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
        
        // Пауза между обновлениями
        usleep(1000000); // 1 секунда
    }
    
    // Завершение работы с ncurses
    endwin();
    
    return 0;
} 