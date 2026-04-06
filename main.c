#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include "glanda_uapi.h"

#define DEVICE_PATH "/dev/glandagpu"
#define H_RES 640
#define V_RES 480
#define VRAM_SIZE (H_RES * V_RES * 4)


#define RGB(r, g, b) ((((r) >> 4) << 8) | (((g) >> 4) << 4) | ((b) >> 4))

void test_mmap_static(int fd) {
    printf("[MMAP] Mapping VRAM to userspace...\n");
    
    uint32_t *vram = mmap(NULL, VRAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (vram == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    printf("[DEBUG] Writing all pixels with random colors...\n");
    int num_pixels = H_RES * V_RES;
    for (int i = 0; i < num_pixels; i++) {
        vram[i] = RGB(rand() % 256, rand() % 256, rand() % 256);
    }
    printf("[DEBUG] All pixels done.\n");
    
    munmap(vram, VRAM_SIZE);
    printf("[MMAP] Done. You should see static noise.\n");
    usleep(500000);
}

void test_clear(int fd) {
    printf("[IOCTL] Testing Hardware Clear...\n");
    struct glanda_clear_cmd cmd = { .color = RGB(0, 0, 50) }; // Dark Blue
    if (ioctl(fd, GLANDA_IOC_CLEAR, &cmd) < 0) perror("Clear failed");
    usleep(200000);
}

void test_interrupt_stress(int fd) {
    printf("[IRQ TEST] Stress testing Interrupt Wait Queue (Starburst)...\n");
    printf("   -> Sending 360 line commands back-to-back.\n");
    printf("   -> If driver hangs here, IRQs are broken.\n");

    int cx = H_RES / 2;
    int cy = V_RES / 2;
    int radius = 200;

    for (int deg = 0; deg < 360; deg += 2) {
        float rad = deg * (3.14159f / 180.0f);
        struct glanda_draw_line_cmd cmd;
        
        cmd.x0 = cx;
        cmd.y0 = cy;
        cmd.x1 = cx + (int)(cos(rad) * radius);
        cmd.y1 = cy + (int)(sin(rad) * radius);
        cmd.color = RGB(255, 200, 0); // Gold

        if (ioctl(fd, GLANDA_IOC_DRAW_LINE, &cmd) < 0) {
            perror("Line ioctl failed");
            break;
        }
    }
    printf("[IRQ TEST] Burst complete.\n");
    sleep(1);
}

void test_animation(int fd) {
    printf("[ANIMATION] Bouncing Box...\n");
    
    int x = 10, y = 10;
    int dx = 5, dy = 5;
    int w = 50, h = 50;
    
    for (int i = 0; i < 300; i++) {
        struct glanda_draw_rect_cmd clear_rect = { x, y, w, h, RGB(0,0,50) }; // Match BG
        ioctl(fd, GLANDA_IOC_DRAW_RECT, &clear_rect);

        x += dx; y += dy;
        if (x <= 0 || x + w >= H_RES) dx = -dx;
        if (y <= 0 || y + h >= V_RES) dy = -dy;

        struct glanda_draw_rect_cmd draw_cmd = { x, y, w, h, RGB(255, 50, 50) };
        ioctl(fd, GLANDA_IOC_DRAW_RECT, &draw_cmd);

        usleep(16000); // 60 FPS
    }
}

int main() {
    srand(time(NULL));

    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open " DEVICE_PATH);
        return 1;
    }

    test_mmap_static(fd);
    test_clear(fd);
    test_interrupt_stress(fd);
    test_animation(fd);

    struct glanda_clear_cmd end_cmd = { .color = 0 };
    ioctl(fd, GLANDA_IOC_CLEAR, &end_cmd);

    printf("Test Sequence Complete.\n");
    close(fd);
    return 0;
}
