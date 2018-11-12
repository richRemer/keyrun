#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <libevdev/libevdev.h>

#define DEVROOT     "/dev/input/"
#define DEVROOT_LEN 12
#define PATH_LEN    (DEVROOT_LEN + NAME_MAX)

int outerr(int, const char*);
struct libevdev* open_device(int);
bool kblike(struct libevdev*);

int main(int argc, char* argv[]) {
    DIR* dir;
    struct dirent* entry;
    char path[PATH_LEN];
    int fd, err;
    struct libevdev* dev = NULL;
    struct input_event ev;
    bool key, rep, alt;

    if (!(dir = opendir("/dev/input"))) {
        return outerr(errno, "cannot enumerate devices");
    }

    // look for keyboard device
    while (entry = readdir(dir)) {
        if (DT_CHR == entry->d_type) {
            sprintf(path, "/dev/input/%s", entry->d_name);

            if (-1 == (fd = open(path, O_RDONLY|O_NONBLOCK))) {
                return outerr(errno, "cannot read device");
            }

            if (dev = open_device(fd)) {
                if (kblike(dev)) break;
                libevdev_free(dev);
                dev = NULL;
            }
        }
    }

    closedir(dir);

    // check if keyboard was found
    if (dev == NULL) {
        return outerr(ENODEV, "could not detect keyboard");
    } else do {
        err = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (err == 0 && ev.type == EV_KEY) switch (ev.code) {
            case KEY_LEFTALT:
                alt = ev.value == 1;
                break;
            case KEY_X:
                if (ev.value == 1 && alt) system("xterm");
                break;
        }
    } while (err == 1 || err == 0 || err == -EAGAIN);

    return 0;
}

int outerr(int errnum, const char* msg) {
    fprintf(stderr, "%s (%s)\n", msg, strerror(errnum));
    return errnum;
}

bool kblike(struct libevdev* dev) {
    return libevdev_has_event_type(dev, EV_KEY)
        && libevdev_has_event_type(dev, EV_REP);
}

struct libevdev* open_device(int fd) {
    struct libevdev* dev = libevdev_new();
    int err;

    if (dev == NULL) {
        errno = ENOMEM;
    } else if (0 > (err = libevdev_set_fd(dev, fd))) {
        libevdev_free(dev);
        dev = NULL;
        errno = -err;
    }

    return dev;
}
