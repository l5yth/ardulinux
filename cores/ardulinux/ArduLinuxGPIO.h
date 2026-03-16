// ArduLinux - Arduino API for Linux
// Copyright (c) 2011-19 Arduino LLC.
// Copyright (c) 2020-23 Geeksville Industries, LLC
// Copyright (c) 2024-26 jp-bennett
// Copyright (c) 2026-27 l5yth
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#pragma once

#include "Common.h"
#include "Utility.h"
#include "logging.h"
#include "Arduino.h"

#include <assert.h>
#include <stdlib.h>

/**
 * The standard interface to a single GPIO line.  A given implementation subclass might be SimGPIOPin or
 * LinuxGPIOPin etc...
 */
class GPIOPinIf
{
public:
    virtual ~GPIOPinIf() = 0;
    virtual pin_size_t getPinNum() const = 0;

    /** Called to read from a pin and if the pin has changed state possibly call an ISR, also changes
   * the mirrored copy of state that is returned for calls to readPin().
   * 
   * If an ISR is attached to this GPIO, call if the speicifed PinStatus matches */
    virtual void refreshIfNeeded() = 0;

    /** Convience function for the common case of mapping to only one GPIO */
    virtual PinStatus readPin() = 0;

    /** Convience function for the common case of mapping to only one GPIO */
    virtual void writePin(PinStatus s) = 0;

    virtual int analogRead() = 0;

    virtual void analogWrite(int v) = 0;

    virtual void setPinMode(PinMode m) = 0;
    
    virtual unsigned long pulseIn(PinStatus state, unsigned long timeout) = 0;

    virtual void attachInterrupt(voidFuncPtr callback, PinStatus mode) = 0;

    virtual void detachInterrupt() = 0;
};

/**
 * An implementation of a single GPIO line.  A given implementation subclass might be SimGPIOPin or
 * LinuxGPIOPin etc...
 */
class GPIOPin : public GPIOPinIf
{
    pin_size_t number;
    String name;

    /** The current pinmode, defaults to INPUT_PULLUP to match most hardware */
    PinMode mode = INPUT_PULLUP;

    /** The last read/written value that reflects the current status of the real hardware */
    PinStatus status = HIGH;

    voidFuncPtr isr = NULL;

    bool silent = false; // If silent, we won't emit logs

protected:

    /// What sorts of edges do we want to invoke the ISR for?
    int8_t isrPinStatus = -1; // -1 or PinStatus

public:
    GPIOPin(pin_size_t n, String _name) : number(n), name(_name) {}
    virtual ~GPIOPin() {}

    pin_size_t getPinNum() const { return number; }

    const char *getName() const { return name.c_str(); }

    /** Convience function for the common case of mapping to only one GPIO */
    PinStatus readPin()
    {
        refreshState(); // Get current hw pin values (might also cause an ISR to run)

        // if (!silent) log(SysGPIO, LogInfo, "readPin(%s, %d, %d)", getName(), getPinNum(), status);

        return status;
    }

    /** Convience function for the common case of mapping to only one GPIO */
    virtual void writePin(PinStatus s)
    {
        // log(SysGPIO, LogDebug, "writePin(%s, %d)", getName(), s);
        assert(s == HIGH || s == LOW); // Don't let user set invalid values
        status = s;

        if (!silent)
            log(SysGPIO, LogInfo, "writePin(%s, %d, %d)", getName(), getPinNum(), s);
    }

    virtual int analogRead()
    {
        notImplemented("gpio:analogRead");
        return 4242;
    }

    virtual void analogWrite(int v) NOT_IMPLEMENTED("gpio:refreshState");

    virtual void attachInterrupt(voidFuncPtr callback, PinStatus mode)
    {
        isr = callback;
        isrPinStatus = mode;
    }

    virtual void detachInterrupt()
    {
        isr = NULL;
        isrPinStatus = -1;
    }

    virtual unsigned long pulseIn(PinStatus state, unsigned long timeout)
    {
        setPinMode(INPUT);
        uint32_t start = micros();
        while(readPin() == state && (micros() - start) < timeout);
        while(readPin() != state && (micros() - start) < timeout);
        start = micros();
        while(readPin() == state && (micros() - start) < timeout);
        return micros() - start;
    }

    virtual void setPinMode(PinMode m)
    {
        mode = m;
        if (!silent)
            log(SysGPIO, LogInfo, "setPinMode(%s, %d, %d)", getName(), getPinNum(), m);
    }

    virtual PinMode getPinMode()
    {
        return mode;
    }

    /** CALLED ONLY BY ARDULINUX special thread
     * If this pin has an ISR attached, poll it and call the ISR if necessary
     */
    void refreshIfNeeded() {
        if(isr)
            refreshState();
    }        

    /** Set silent mode
     * @return this for easy chaining
     */
    GPIOPin *setSilent(bool s = true)
    {
        silent = s;
        return this;
    }

private:
    /**
     * Invoke the attached ISR if the edge/level condition is satisfied.
     *
     * Compares @p oldState (previous pin value) with @p newState (current
     * value) against the registered isrPinStatus trigger mode:
     *  - HIGH/LOW   — level-triggered; fires whenever the pin is at that level
     *  - RISING     — fires on LOW→HIGH transition
     *  - FALLING    — fires on HIGH→LOW transition
     *  - CHANGE     — fires on any state change
     *
     * @param oldState Pin value before the current poll cycle.
     * @param newState Pin value after the current poll cycle.
     */
    void callISR(PinStatus oldState, PinStatus newState)
    {
        auto s = isrPinStatus;
        // Evaluate all trigger conditions; non-zero PinStatus values are truthy.
        if ((s == HIGH && newState) ||
            (s == LOW && !newState) ||
            (s == RISING && !oldState && newState) ||
            (s == FALLING && oldState && !newState) ||
            (s == CHANGE && !!oldState != !!newState))
        {
            if(!silent)
                log(SysGPIO, LogDebug, "GPIOPin::callISR(%s, %d)", getName(), getPinNum());
            assert(isr);
            isr();
        }
    }

    /**
     * Read the current hardware state, update the cached status, and trigger
     * any attached ISR if the edge/level condition is met.
     *
     * Skipped when the pin is configured as OUTPUT because output pins do not
     * receive external signals and their state is set by writePin() instead.
     */
    void refreshState()
    {
        if (mode != OUTPUT)
        {
            auto newState = readPinHardware();
            if(!silent)
                log(SysGPIO, LogDebug, "refreshState(%s, %d)", getName(), newState);
            auto oldState = status;
            status = newState;  // Update cached value before calling ISR
            callISR(oldState, newState);
        }
    }

protected:
    /// Return the current the low level hardware for this pin, used to set pin status and (later) trigger ISRs
    virtual PinStatus readPinHardware()
    {
        // default to assume no change
        return status;
    }
};

/**
 * A software-only GPIO pin that never touches real hardware.
 *
 * Used as the default for every slot in the pin table until the application
 * replaces specific pins with LinuxGPIOPin instances via gpioBind().
 * Reads return the last written value; writes are silently accepted.
 */
class SimGPIOPin : public GPIOPin
{
public:
    /** Construct a simulated pin with the given Arduino pin number and name. */
    SimGPIOPin(pin_size_t n, String _name) : GPIOPin(n, _name) {}
};

/**
 * Initialise the global GPIO pin table.
 *
 * Allocates @p _num_gpios SimGPIOPin instances indexed 0 … _num_gpios-1.
 * Must be called once before any GPIO API function.  Calling it a second
 * time appends additional SimGPIOPin instances without resetting existing
 * ones — do not call more than once per process.
 *
 * @param _num_gpios Number of GPIO slots to allocate (default: 64).
 */
void gpioInit(int _num_gpios = 64);

/**
 * Poll all GPIO pins that have ISRs attached.
 *
 * Called from the main loop (and from delay() when real hardware is present)
 * to simulate interrupt-driven behaviour.  For each pin with an attached ISR,
 * reads the current hardware state and fires the ISR if the edge condition is
 * met.
 */
void gpioIdle();

/**
 * Replace the SimGPIOPin at slot p->getPinNum() with a real hardware pin.
 *
 * Takes ownership of @p p.  Also sets the global @p realHardware flag, which
 * disables the 100 ms loop sleep in ardulinux_main() so ISRs are polled at
 * full speed.
 *
 * @param p Heap-allocated GPIOPinIf implementation to install.
 */
void gpioBind(GPIOPinIf *p);

/**
 * True once at least one real hardware pin has been registered via gpioBind().
 *
 * When false, the main loop adds a 100 ms sleep to avoid burning CPU on
 * pure-simulation workloads.
 */
extern bool realHardware;