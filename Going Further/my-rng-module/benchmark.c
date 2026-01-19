/*
 * Benchmark pour mesurer les performances du RNG
 * Compare les performances 32-bit vs 64-bit
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <stdint.h>

#define RAND_IOCTL    0x80047101  // MY_RNG_IOCTL_RAND (32-bit)
#define RAND64_IOCTL  0x80087102  // MY_RNG_IOCTL_RAND64 (64-bit)
#define SEED_IOCTL    0x40047101  // MY_RNG_IOCTL_SEED

#define NUM_ITERATIONS 1000000    // 1 million d'appels

void benchmark_32bit(int fd) {
    unsigned int random_number;
    struct timespec start, end;
    double elapsed_time;
    unsigned long long total_bytes = 0;

    printf("\n=== Benchmark 32-bit RNG ===\n");
    printf("Nombre d'itérations : %d\n", NUM_ITERATIONS);

    // Mesurer le temps
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        if (ioctl(fd, RAND_IOCTL, &random_number)) {
            perror("ioctl rand 32-bit");
            return;
        }
        total_bytes += sizeof(random_number);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculer le temps écoulé
    elapsed_time = (end.tv_sec - start.tv_sec) + 
                   (end.tv_nsec - start.tv_nsec) / 1e9;

    // Afficher les résultats
    printf("Temps écoulé       : %.3f secondes\n", elapsed_time);
    printf("Opérations/sec     : %.0f ops/s\n", NUM_ITERATIONS / elapsed_time);
    printf("Données générées   : %.2f MB\n", total_bytes / (1024.0 * 1024.0));
    printf("Throughput         : %.2f MB/s\n", 
           (total_bytes / (1024.0 * 1024.0)) / elapsed_time);
    printf("Latence moyenne    : %.2f µs/op\n", 
           (elapsed_time * 1e6) / NUM_ITERATIONS);
}

void benchmark_64bit(int fd) {
    unsigned long long random_number;
    struct timespec start, end;
    double elapsed_time;
    unsigned long long total_bytes = 0;

    printf("\n=== Benchmark 64-bit RNG ===\n");
    printf("Nombre d'itérations : %d\n", NUM_ITERATIONS);

    // Mesurer le temps
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        if (ioctl(fd, RAND64_IOCTL, &random_number)) {
            perror("ioctl rand 64-bit");
            return;
        }
        total_bytes += sizeof(random_number);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculer le temps écoulé
    elapsed_time = (end.tv_sec - start.tv_sec) + 
                   (end.tv_nsec - start.tv_nsec) / 1e9;

    // Afficher les résultats
    printf("Temps écoulé       : %.3f secondes\n", elapsed_time);
    printf("Opérations/sec     : %.0f ops/s\n", NUM_ITERATIONS / elapsed_time);
    printf("Données générées   : %.2f MB\n", total_bytes / (1024.0 * 1024.0));
    printf("Throughput         : %.2f MB/s\n", 
           (total_bytes / (1024.0 * 1024.0)) / elapsed_time);
    printf("Latence moyenne    : %.2f µs/op\n", 
           (elapsed_time * 1e6) / NUM_ITERATIONS);
}

void test_correctness(int fd) {
    unsigned int seed = 0x12345678;
    unsigned int rand32;
    unsigned long long rand64;

    printf("\n=== Test de Correction ===\n");

    // Test 32-bit
    if (ioctl(fd, SEED_IOCTL, &seed)) {
        perror("ioctl seed");
        return;
    }

    if (ioctl(fd, RAND_IOCTL, &rand32)) {
        perror("ioctl rand 32-bit");
        return;
    }
    printf("32-bit random: 0x%08x (%u)\n", rand32, rand32);

    // Test 64-bit
    if (ioctl(fd, RAND64_IOCTL, &rand64)) {
        perror("ioctl rand 64-bit");
        return;
    }
    printf("64-bit random: 0x%016llx (%llu)\n", rand64, rand64);

    // Générer quelques nombres pour vérifier
    printf("\nQuelques nombres 64-bit:\n");
    for (int i = 0; i < 5; i++) {
        if (ioctl(fd, RAND64_IOCTL, &rand64)) {
            perror("ioctl rand 64-bit");
            return;
        }
        printf("  %d: 0x%016llx (%llu)\n", i+1, rand64, rand64);
    }
}

int main() {
    int fd = open("/dev/my_rng_driver", O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device file");
        return -1;
    }

    printf("       RNG Performance Benchmark - 32-bit vs 64-bit      \n");


    // Test de correction
    test_correctness(fd);

    // Benchmark 32-bit
    benchmark_32bit(fd);

    // Benchmark 64-bit
    benchmark_64bit(fd);


    close(fd);
    return 0;
}
