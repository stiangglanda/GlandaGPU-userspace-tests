#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include <drm/drm.h>
#include <drm/drm_mode.h>

#define DEVICE_PATH "/dev/dri/card0"
#define H_RES 640
#define V_RES 480
#define VRAM_SIZE (H_RES * V_RES * 4)

#define RGB(r, g, b) ((((r) >> 4) << 8) | (((g) >> 4) << 4) | ((b) >> 4))

void test_mmap_static(int fd, uint32_t crtc_id, uint32_t connector_id) {
    struct drm_mode_create_dumb create = {0};
    struct drm_mode_map_dumb map_req = {0};
    struct drm_mode_fb_cmd add_fb = {0};
    struct drm_mode_crtc set_crtc = {0};
    
    struct drm_mode_fb_dirty_cmd dirty = {0}; 
    struct drm_mode_destroy_dumb destroy = {0};
    
    uint32_t *vram;

    struct drm_mode_modeinfo mode = {
        .clock = 25175,
        .hdisplay = 640, .hsync_start = 656, .hsync_end = 752, .htotal = 800,
        .vdisplay = 480, .vsync_start = 490, .vsync_end = 492, .vtotal = 525,
        .vrefresh = 60,
        .flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
        .name = "640x480",
    };

    printf("Allocating GEM Dumb Buffer...\n");
    create.width = H_RES;
    create.height = V_RES;
    create.bpp = 32;
    if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
        perror("Create Dumb failed");
        return;
    }

    printf("Registering Buffer as DRM Framebuffer (ADDFB)...\n");
    add_fb.width = H_RES;
    add_fb.height = V_RES;
    add_fb.pitch = H_RES * 4;
    add_fb.bpp = 32;
    add_fb.depth = 24;
    add_fb.handle = create.handle;
    if (ioctl(fd, DRM_IOCTL_MODE_ADDFB, &add_fb) < 0) {
        perror("Add FB failed");
        goto err_destroy;
    }

    printf("Activating Framebuffer on CRTC (SETCRTC)...\n");
    set_crtc.crtc_id = crtc_id;
    set_crtc.fb_id = add_fb.fb_id;
    
    set_crtc.set_connectors_ptr = (uint64_t)(uintptr_t)&connector_id;
    set_crtc.count_connectors = 1;
    set_crtc.mode = mode;
    set_crtc.mode_valid = 1;
    if (ioctl(fd, DRM_IOCTL_MODE_SETCRTC, &set_crtc) < 0) {
        perror("Set CRTC failed");
        goto err_rm_fb;
    }

    printf("Mapping Buffer and writing static noise...\n");
    map_req.handle = create.handle;
    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) < 0) {
        perror("Map Dumb failed");
        goto err_rm_fb;
    }

    vram = mmap(NULL, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_req.offset);
    if (vram == MAP_FAILED) {
        perror("mmap failed");
        goto err_rm_fb;
    }

    int num_pixels = H_RES * V_RES;
    for (int i = 0; i < num_pixels; i++) {
        uint8_t r = rand() % 256;
        uint8_t g = rand() % 256;
        uint8_t b = rand() % 256;
        
        vram[i] = (r << 16) | (g << 8) | b;
    }

    printf("Flushing Shadow Buffer to FPGA VRAM (DIRTYFB)...\n");
    dirty.fb_id = add_fb.fb_id;
    if (ioctl(fd, DRM_IOCTL_MODE_DIRTYFB, &dirty) < 0) {
        perror("Dirty FB failed");
    }

    printf("Display complete. Showing noise for 1 second...\n");
    sleep(1);

    munmap(vram, create.size);

err_rm_fb:
    ioctl(fd, DRM_IOCTL_MODE_RMFB, &add_fb.fb_id);

err_destroy:
    destroy.handle = create.handle;
    ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    uint32_t crtc_id = 34;
    uint32_t connector_id = 36;

    if (argc == 3) {
        crtc_id = strtoul(argv[1], NULL, 10);
        connector_id = strtoul(argv[2], NULL, 10);
    } else {
        printf("Using default IDs: CRTC=%u, Connector=%u\n", crtc_id, connector_id);
        printf("Note: If SETCRTC fails, pass correct IDs via: %s <crtc_id> <connector_id>\n", argv[0]);
    }

    printf("Opening DRM device " DEVICE_PATH "...\n");
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open " DEVICE_PATH);
        return 1;
    }

    test_mmap_static(fd, crtc_id, connector_id);

    printf("Test Sequence Complete.\n");
    close(fd);
    return 0;
}