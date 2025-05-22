#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <getopt.h>

#define DEFAULT_NUM_THREADS 8
#define DEFAULT_ARRAY_SIZE (1024 * 1024 * 16)
#define CACHE_LINE_SIZE 64
#define MONITOR_INTERVAL 1

// Глобальные настройки
typedef struct {
    int num_threads;
    size_t array_size;
    int monitor_interval;
} Config;

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
    size_t chunk_size = array_size / data->num_threads;
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
        
        sleep(monitor->monitor_interval);
    }
    return NULL;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -t, --threads NUM    Number of threads (default: %d)\n", DEFAULT_NUM_THREADS);
    printf("  -s, --size SIZE      Array size in MB (default: %zu)\n", DEFAULT_ARRAY_SIZE / (1024 * 1024));
    printf("  -i, --interval SEC   Monitoring interval in seconds (default: %d)\n", MONITOR_INTERVAL);
    printf("  -h, --help          Show this help message\n");
}

int parse_args(int argc, char* argv[], Config* config) {
    static struct option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"size", required_argument, 0, 's'},
        {"interval", required_argument, 0, 'i'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    // Устанавливаем значения по умолчанию
    config->num_threads = DEFAULT_NUM_THREADS;
    config->array_size = DEFAULT_ARRAY_SIZE;
    config->monitor_interval = MONITOR_INTERVAL;

    int opt;
    while ((opt = getopt_long(argc, argv, "t:s:i:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 't':
                config->num_threads = atoi(optarg);
                break;
            case 's':
                config->array_size = atoi(optarg) * 1024 * 1024;
                break;
            case 'i':
                config->monitor_interval = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    Config config;
    if (parse_args(argc, argv, &config) != 0) {
        return 1;
    }

    pthread_t threads[config.num_threads];
    pthread_t monitor_thread;
    ThreadData thread_data[config.num_threads];
    MonitorData monitor_data = {0};
    volatile int* shared_array;
    size_t total_size = config.array_size * sizeof(int);
    
    printf("Configuration:\n");
    printf("  Threads: %d\n", config.num_threads);
    printf("  Array size: %zu MB\n", config.array_size / (1024 * 1024));
    printf("  Monitor interval: %d seconds\n", config.monitor_interval);
    
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
    for(size_t i = 0; i < config.array_size; i++) {
        shared_array[i] = i;
    }
    
    printf("Successfully allocated %zu MB of shared memory\n", total_size / (1024 * 1024));
    printf("Starting stress test with %d threads...\n", config.num_threads);
    
    // Устанавливаем высокий приоритет
    if (nice(-20) == -1) {
        printf("Warning: Could not set high priority: %s\n", strerror(errno));
    }
    
    // Создаем потоки
    for(int i = 0; i < config.num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].array = shared_array;
        thread_data[i].array_size = config.array_size;
        
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
    for(int i = 0; i < config.num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Останавливаем мониторинг
    monitor_data.running = 0;
    pthread_join(monitor_thread, NULL);
    
    free((void*)shared_array);
    return 0;
} 