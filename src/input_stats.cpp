#include "input_stats.h"
#include <X11/Xlib.h>

namespace input_stats {

namespace {
    Display* display = nullptr;
}

bool initialize() {
    display = XOpenDisplay(nullptr);
    return display != nullptr;
}

int poll_keystrokes() {
    if (!display) {
        return 0;
    }

    int count = 0;
    XEvent event;

    // Process all pending events without blocking
    while (XPending(display) > 0) {
        XNextEvent(display, &event);
        if (event.type == KeyPress) {
            count++;
        }
    }

    return count;
}

void shutdown() {
    if (display) {
        XCloseDisplay(display);
        display = nullptr;
    }
}

} // namespace input_stats
