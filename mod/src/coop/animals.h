#pragma once
// Phase 4 shared animals. Game thread only. Design: re/notes/lead-animals.md.
#include "net/protocol.h"

#include <cstdint>

namespace dh2::coop::animals {

// Host, 20 Hz: build AnimalStates from Animals() and broadcast. No-op until 50 ms have passed.
void HostTick(uint32_t nowMs);
// Client: append one snapshot. localMs is this process's steady clock at arrival.
void OnAnimalStates(const net::AnimalStates& states, uint32_t localMs);
// Host: apply a client's hit with FuncInflictDamage.
void OnAnimalHit(const net::AnimalHit& hit);
// Drop buffers and unfreeze any animal this process was driving.
void Reset();
// `active` is a host with at least one peer, or a connected client. The solo spike stays off while active.
void SetSessionActive(bool active, bool hosting);

}  // namespace dh2::coop::animals
