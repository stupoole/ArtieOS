// ArtOS - hobby operating system by Artie Poole
// Copyright (C) 2025 Stuart Forbes Poole <artiepoole>
//
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <https://www.gnu.org/licenses/>

//
// Created by artypoole on 09/07/24.
//

#include "kernel.h"

#include <EventQueue.h>
#include <Scheduler.h>
#include "SMBIOS.h"

#include "logging.h"
#include "PIT.h"
#include "VideoGraphicsArray.h"
#include "TSC.h"

// u64 clock_rate = 0;

void write_standard([[maybe_unused]] const char* buffer, [[maybe_unused]] unsigned long len)
{
    WRITE(buffer, len);
}

void write_error([[maybe_unused]] const char* buffer, [[maybe_unused]] unsigned long len)
{
    // // todo: implement the propagation of colour so that this can be overridden to use red for errors or something.
    // auto& term = Terminal::get();
    // term.write(buffer, len, COLOR_RED);

    WRITE(buffer, len);
}

tm* get_time()
{
    return RTC::get().getTime();
}


time_t get_epoch_time()
{
    return RTC::get().epochTime();
}

extern "C"
void _exit([[maybe_unused]] int status)
{
    Scheduler::exit(status);
}

u64 get_clock_rate_hz()
{
    return SMBIOS_get_CPU_clock_rate_hz();
}

u64 get_current_clock()
{
    return TSC_get_ticks();
}

uint32_t get_tick_s()
{
    return TSC_get_ticks() / (SMBIOS_get_CPU_clock_rate_hz());
}

uint32_t get_tick_ms()
{
    return TSC_get_ticks() / (SMBIOS_get_CPU_clock_rate_hz() / 1000);
}

uint32_t get_tick_us()
{
    return TSC_get_ticks() / (SMBIOS_get_CPU_clock_rate_mhz());
}

uint32_t get_tick_ns()
{
    return TSC_get_ticks() / (SMBIOS_get_CPU_clock_rate_mhz() / 1000);
}

// TODO: replace PIT_sleep* with scheduler sleep
void sleep_s(const u32 s)
{
    const u32 ms = s * 1000;
    // if s*1000 out of bounds for u32 then handle that by sleeping for s lots of milliseconds a thousand times.
    if (ms < s) { for (u32 i = 0; i < 1000; i++) PIT_sleep_ms(s); }
    PIT_sleep_ms(ms);
}

void sleep_ms(const u32 ms)
{
    Scheduler::sleep_ms(ms);
    // PIT_sleep_ms(ms);
}

void sleep_ns(const u32 ns)
{
    const u32 start = get_tick_ns();
    while (get_tick_ns() - start < ns)
    {
    };
}

void sleep_us(const u32 us)
{
    const u32 start = get_tick_us();
    while (get_tick_us() - start < us)
    {
    };
}


void pause_exec(const u32 ms)
{
    Scheduler::sleep_ms(ms);
}

bool probe_pending_events()
{
    return Scheduler::getCurrentProcessEventQueue()->pendingEvents();
}

event_t get_next_event()
{
    // TODO: this should only get events associated with the correct event queue.
    // There should not be a generic system event queue but instead there should be one associated with each processes
    return Scheduler::getCurrentProcessEventQueue()->getEvent();
}

void draw_screen_region(const u32* frame_buffer)
{
    VideoGraphicsArray::get().drawRegion(frame_buffer);
}
