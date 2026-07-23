#ifndef INPUT_STATS_H
#define INPUT_STATS_H

namespace input_stats {

// Initializes the X11 display connection.
// Returns true on success, false on error.
bool initialize();

// Returns the number of key presses since the last call.
// Returns 0 if no events or on error.
int poll_keystrokes();

// Closes the X11 display connection.
void shutdown();

} // namespace input_stats

#endif // INPUT_STATS_H
