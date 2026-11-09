#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <errno.h>
#include <math.h>
#include <os/log.h>
#include <spawn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/wait.h>

typedef struct { float x, y; } TouchPoint;

// MultitouchSupport callback ABI. Only ID, state, and normalized position are read.
typedef struct {
    int32_t frame;
    double timestamp;
    int32_t id, state, finger_id, hand_id;
    TouchPoint position;
    unsigned char unused[56];
} Contact;
_Static_assert(sizeof(Contact) == 96 && offsetof(Contact, position) == 32,
               "Unexpected MultitouchSupport contact layout");

typedef struct {
    bool tracking, fired;
    int32_t ids[5];
    TouchPoint center;
    float radius;
    double start, last_frame;
} Pinch;

static bool detect_pinch(Pinch *pinch, const Contact *contacts, int count, double now) {
    if (now < pinch->last_frame || now - pinch->last_frame > 0.30) {
        *pinch = (Pinch){0};
    }
    pinch->last_frame = now;

    TouchPoint points[5];
    int32_t ids[5];
    int active = 0;
    for (int i = 0; contacts && i < count; i++) {
        if (contacts[i].state != 3 && contacts[i].state != 4) continue;
        if (active == 5) {
            pinch->tracking = false;
            return false;
        }
        points[active] = contacts[i].position;
        int slot = active++;
        while (slot > 0 && ids[slot - 1] > contacts[i].id) {
            ids[slot] = ids[slot - 1];
            slot--;
        }
        ids[slot] = contacts[i].id;
    }
    if (active != 5) {
        pinch->tracking = false;
        if (active == 0) pinch->fired = false;
        return false;
    }
    if (pinch->fired) return false;

    TouchPoint center = {0};
    for (int i = 0; i < 5; i++) {
        if (!isfinite(points[i].x) || !isfinite(points[i].y)) return false;
        center.x += points[i].x / 5;
        center.y += points[i].y / 5;
    }
    float radius = 0;
    for (int i = 0; i < 5; i++) {
        radius += hypotf(points[i].x - center.x, points[i].y - center.y) / 5;
    }

    bool same_fingers = pinch->tracking;
    for (int i = 0; i < 5; i++) {
        if (ids[i] != pinch->ids[i]) same_fingers = false;
        pinch->ids[i] = ids[i];
    }
    if (!same_fingers || hypotf(center.x - pinch->center.x, center.y - pinch->center.y) > 0.13f) {
        pinch->tracking = true;
        pinch->center = center;
        pinch->radius = radius;
        pinch->start = now;
        return false;
    }
    if (now - pinch->start < 0.05 || now - pinch->start > 1.4) return false;
    if (radius > pinch->radius) pinch->radius = radius;
    if (pinch->radius < 0.13f || pinch->radius - radius < 0.045f || radius > pinch->radius * 0.80f) {
        return false;
    }
    pinch->fired = true;
    return true;
}

#ifndef TEST_PINCH
extern char **environ;
static Pinch pinch;

static void open_launchos(void *context) {
    (void)context;
    pid_t pid;
    char *const arguments[] = {"/usr/bin/open", "-a", "/Applications/LaunchOS.app", NULL};
    int error = posix_spawn(&pid, arguments[0], NULL, NULL, arguments, environ);
    if (error != 0) {
        os_log_error(OS_LOG_DEFAULT, "LaunchOS gesture: open could not start (%d)", error);
        return;
    }
    int status;
    pid_t child;
    do { child = waitpid(pid, &status, 0); } while (child == -1 && errno == EINTR);
    if (child == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        os_log(OS_LOG_DEFAULT, "LaunchOS gesture: application opened");
    } else {
        os_log_error(OS_LOG_DEFAULT, "LaunchOS gesture: open failed");
    }
}

static int receive_frame(void *device, const Contact *contacts, int count, double time, int frame) {
    (void)device;
    (void)frame;
    if (detect_pinch(&pinch, contacts, count, time)) {
        dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW, 170 * NSEC_PER_MSEC),
                         dispatch_get_main_queue(), NULL, open_launchos);
    }
    return 0;
}

int main(void) {
    void *framework = dlopen("/System/Library/PrivateFrameworks/MultitouchSupport.framework/MultitouchSupport",
                             RTLD_NOW | RTLD_LOCAL);
    if (!framework) return 1;
    CFArrayRef (*create_list)(void) = dlsym(framework, "MTDeviceCreateList");
    bool (*is_builtin)(const void *) = dlsym(framework, "MTDeviceIsBuiltIn");
    void (*register_callback)(const void *, __typeof__(&receive_frame)) =
        dlsym(framework, "MTRegisterContactFrameCallback");
    int (*start)(const void *, int) = dlsym(framework, "MTDeviceStart");
    if (!create_list || !is_builtin || !register_callback || !start) return 2;

    CFArrayRef devices = create_list();
    if (!devices) return 3;
    for (CFIndex i = 0; i < CFArrayGetCount(devices); i++) {
        const void *device = CFArrayGetValueAtIndex(devices, i);
        if (!is_builtin(device)) continue;
        register_callback(device, receive_frame);
        if (start(device, 0) != 0) return 4;
        os_log(OS_LOG_DEFAULT, "LaunchOS gesture: built-in trackpad ready");
        dispatch_main();
    }
    return 3;
}
#endif
