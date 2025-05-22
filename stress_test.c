#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

#define NUM_THREADS 8
#define ARRAY_SIZE (1024 * 1024 * 16)  // Уменьшаем до 64MB для начала
#define CACHE_LINE_SIZE 64
#define MONITOR_INTERVAL 1  // Интервал мониторинга в секундах

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    volatile int* array;
    size_t array_size;
} ThreadData;

// Структура для мониторинга
typedef struct {
    double cpu_usage;
    long mem_usage;
    int running;
} MonitorData;

// Функция для нагрузки процессора и кэша
void* cpu_stress(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    volatile int* array = data->array;
    int thread_id = data->thread_id;
    size_t array_size = data->array_size;
    
    // Выделяем память для локального массива
    volatile int* local_array = (int*)malloc(array_size * sizeof(int));
    if (!local_array) {
        printf("Thread %d: Failed to allocate %zu bytes\n", 
               thread_id, array_size * sizeof(int));
        return NULL;
    }
    
    // Инициализация с разными значениями для каждого потока
    for(size_t i = 0; i < array_size; i++) {
        local_array[i] = i * (thread_id + 1);
    }
    
    printf("Thread %d started with %zu MB local array\n", 
           thread_id, (array_size * sizeof(int)) / (1024 * 1024));
    
    // Определяем размер части массива для каждого потока
    size_t chunk_size = array_size / NUM_THREADS;
    size_t start_idx = thread_id * chunk_size;
    size_t end_idx = start_idx + chunk_size;
    
    while(1) {
        // Интенсивная работа с кэшем через случайный доступ
        for(size_t i = start_idx; i < end_idx; i++) {
            size_t idx = ((i * 16807) % chunk_size) + start_idx;
            local_array[idx] = local_array[(idx + CACHE_LINE_SIZE) % end_idx] * 3;
            local_array[idx] = local_array[idx] * local_array[idx] % 1000000007;
            
            // Добавляем больше математических операций
            for(int j = 0; j < 10; j++) {
                local_array[idx] = (local_array[idx] * 31 + thread_id) % 1000000007;
            }
            
            // Принудительная запись в память
            array[idx] = local_array[idx];
        }
        
        // Работа с разными участками кэша
        for(int stride = 1; stride < CACHE_LINE_SIZE; stride *= 2) {
            for(size_t i = start_idx; i < end_idx - stride; i += stride) {
                local_array[i] = (local_array[i] + local_array[i + stride]) % 1000000007;
            }
        }
    }
    
    free((void*)local_array);
    return NULL;
}

// Функция мониторинга
void* monitor_resources(void* arg) {
    MonitorData* monitor = (MonitorData*)arg;
    struct rusage usage;
    struct sysinfo si;
    
    while(monitor->running) {
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            monitor->cpu_usage = (double)usage.ru_utime.tv_sec + 
                                (double)usage.ru_utime.tv_usec / 1000000.0;
        }
        
        if (sysinfo(&si) == 0) {
            monitor->mem_usage = si.totalram - si.freeram;
        }
        
        printf("\rCPU Usage: %.2f%% | Memory Usage: %.2f MB", 
               monitor->cpu_usage * 100.0,
               monitor->mem_usage / (1024.0 * 1024.0));
        fflush(stdout);
        
        sleep(MONITOR_INTERVAL);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_t monitor_thread;
    ThreadData thread_data[NUM_THREADS];
    MonitorData monitor_data = {0};
    volatile int* shared_array;
    size_t total_size = ARRAY_SIZE * sizeof(int);
    
    printf("Attempting to allocate %zu MB of memory...\n", total_size / (1024 * 1024));
    
    // Выделяем память с выравниванием по размеру кэш-линии
    if(posix_memalign((void**)&shared_array, CACHE_LINE_SIZE, total_size) != 0) {
        printf("Failed to allocate %zu bytes: %s\n", total_size, strerror(errno));
        return 1;
    }
    
    // Проверяем успешность выделения памяти
    if (!shared_array) {
        printf("Memory allocation failed!\n");
        return 1;
    }
    
    // Инициализируем массив
    for(size_t i = 0; i < ARRAY_SIZE; i++) {
        shared_array[i] = i;
    }
    
    printf("Successfully allocated %zu MB of shared memory\n", total_size / (1024 * 1024));
    printf("Starting stress test with %d threads...\n", NUM_THREADS);
    
    // Устанавливаем высокий приоритет
    if (nice(-20) == -1) {
        printf("Warning: Could not set high priority: %s\n", strerror(errno));
    }
    
    // Создаем потоки
    for(int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].array = shared_array;
        thread_data[i].array_size = ARRAY_SIZE;
        
        if(pthread_create(&threads[i], NULL, cpu_stress, &thread_data[i]) != 0) {
            printf("Failed to create thread %d: %s\n", i, strerror(errno));
            // Очищаем ресурсы перед выходом
            free((void*)shared_array);
            return 1;
        }
    }
    
    // Запускаем мониторинг
    monitor_data.running = 1;
    if(pthread_create(&monitor_thread, NULL, monitor_resources, &monitor_data) != 0) {
        printf("Failed to create monitor thread: %s\n", strerror(errno));
    }
    
    printf("All threads started. Press Ctrl+C to stop.\n");
    
    // Ждем завершения потоков
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Останавливаем мониторинг
    monitor_data.running = 0;
    pthread_join(monitor_thread, NULL);
    
    free((void*)shared_array);
    return 0;
} 