#pragma once

#include "quantum.h"

// One-shot diagonal wipe played on switching into the audio_visualizer
// effect, before external frames start arriving.
void     audio_visualizer_trigger(void);
bool     audio_visualizer_running(void);
uint32_t audio_visualizer_elapsed(void);
void     audio_visualizer_stop(void);
