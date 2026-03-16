// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <catch2/catch_test_macros.hpp>
#include "ArduLinuxGPIO.h"

/**
 * A GPIOPin subclass that lets tests inject a hardware state without
 * touching real GPIO lines. Simulates edge transitions by updating hwState.
 */
class TestGPIOPin : public GPIOPin
{
    PinStatus hwState;

public:
    TestGPIOPin(pin_size_t n, PinStatus initial = HIGH)
        : GPIOPin(n, "test"), hwState(initial) { setSilent(); }

    void setHwState(PinStatus s) { hwState = s; }

protected:
    PinStatus readPinHardware() override { return hwState; }
};

// ─── SimGPIOPin defaults ──────────────────────────────────────────────────────

TEST_CASE("gpioInit populates pins with SimGPIOPins", "[gpio]") {
    gpioInit(4);
    // Default PinStatus is HIGH and mode is INPUT_PULLUP (see GPIOPin)
    CHECK(digitalRead(0) == HIGH);
    CHECK(digitalRead(3) == HIGH);
}

// ─── Output write / read round-trip ──────────────────────────────────────────

TEST_CASE("digitalWrite / digitalRead round-trip on output pin", "[gpio]") {
    gpioInit(4);
    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);
    CHECK(digitalRead(0) == LOW);
    digitalWrite(0, HIGH);
    CHECK(digitalRead(0) == HIGH);
}

// ─── gpioBind ────────────────────────────────────────────────────────────────

TEST_CASE("gpioBind replaces sim pin and sets realHardware", "[gpio]") {
    gpioInit(4);
    realHardware = false;
    CHECK(realHardware == false);

    gpioBind(new TestGPIOPin(0, LOW));
    CHECK(realHardware == true);
    // The bound pin starts LOW in hardware
    CHECK(digitalRead(0) == LOW);
}

// ─── ISR – FALLING edge ──────────────────────────────────────────────────────

TEST_CASE("ISR fires on FALLING edge", "[gpio]") {
    gpioInit(4);
    auto *pin = new TestGPIOPin(1, HIGH); // starts HIGH
    gpioBind(pin);

    // Synchronise internal status with hw state (HIGH)
    digitalRead(1);

    static bool fired;
    fired = false;
    // Captureless lambda is convertible to voidFuncPtr
    attachInterrupt(1, [] { fired = true; }, FALLING);

    pin->setHwState(LOW);       // simulate falling edge
    pin->refreshIfNeeded();     // poll (as gpioIdle() would)
    CHECK(fired == true);

    detachInterrupt(1);
}

// ─── ISR – RISING edge ───────────────────────────────────────────────────────

TEST_CASE("ISR fires on RISING edge", "[gpio]") {
    gpioInit(4);
    auto *pin = new TestGPIOPin(2, LOW); // starts LOW
    gpioBind(pin);

    // Sync: first read brings internal status to LOW
    digitalRead(2);

    static bool fired;
    fired = false;
    attachInterrupt(2, [] { fired = true; }, RISING);

    pin->setHwState(HIGH);      // simulate rising edge
    gpioIdle();
    CHECK(fired == true);

    detachInterrupt(2);
}

// ─── ISR – CHANGE ────────────────────────────────────────────────────────────

TEST_CASE("ISR fires on CHANGE (both edges)", "[gpio]") {
    gpioInit(4);
    auto *pin = new TestGPIOPin(3, HIGH);
    gpioBind(pin);
    digitalRead(3); // sync to HIGH

    static int count;
    count = 0;
    attachInterrupt(3, [] { count++; }, CHANGE);

    pin->setHwState(LOW);
    pin->refreshIfNeeded();
    pin->setHwState(HIGH);
    pin->refreshIfNeeded();
    CHECK(count == 2);

    detachInterrupt(3);
}

// ─── detachInterrupt ─────────────────────────────────────────────────────────

TEST_CASE("detachInterrupt stops ISR from firing", "[gpio]") {
    gpioInit(4);
    auto *pin = new TestGPIOPin(0, HIGH);
    gpioBind(pin);
    digitalRead(0); // sync

    static bool fired;
    fired = false;
    attachInterrupt(0, [] { fired = true; }, FALLING);
    detachInterrupt(0);

    pin->setHwState(LOW);
    pin->refreshIfNeeded();
    CHECK(fired == false);
}
