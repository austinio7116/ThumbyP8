/*
 * ThumbyP8 — Lua bindings for the PICO-8 API.
 *
 * Phase 1+2 surface: drawing primitives, sprites, map, input,
 * a few math helpers. We do NOT yet bind audio (Phase 4) or
 * persistent storage (Phase 6).
 *
 * The bindings need to know which `p8_machine` and `p8_input`
 * they're talking to. We stash pointers in the Lua registry
 * under fixed light-userdata keys when the API is installed.
 */
#ifndef THUMBYP8_API_H
#define THUMBYP8_API_H

#include "p8.h"
#include "p8_machine.h"
#include "p8_input.h"

void p8_api_install(p8_vm *vm, p8_machine *machine, p8_input *input);

/* Helper used by the host runner: invoke a global function `name`
 * with no args. Returns 0 if the function doesn't exist or returned
 * cleanly; nonzero on Lua error. Errors print to stderr. */
int p8_api_call_optional(p8_vm *vm, const char *name);

#endif
