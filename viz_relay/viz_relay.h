#pragma once

#include "quantum.h"

// One-shot diagonal wipe played on switching into the viz_frame effect,
// before external frames start arriving.
void     viz_relay_trigger(void);
bool     viz_relay_running(void);
uint32_t viz_relay_elapsed(void);
void     viz_relay_stop(void);
