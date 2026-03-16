// ArduLinux - Arduino API for Linux
// Copyright (c) 2026-27 l5yth
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <catch2/catch_test_macros.hpp>
#include "SPIChip.h"

// ─── SimSPIChip ──────────────────────────────────────────────────────────────

TEST_CASE("SimSPIChip::transfer returns 0 (success)", "[spi]") {
    SimSPIChip chip;
    uint8_t out[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t in[4]  = {};
    CHECK(chip.transfer(out, in, sizeof(out)) == 0);
}

TEST_CASE("SimSPIChip::transfer handles null buffers", "[spi]") {
    SimSPIChip chip;
    CHECK(chip.transfer(nullptr, nullptr, 8) == 0);
}

TEST_CASE("SimSPIChip::isSimulated returns true", "[spi]") {
    SimSPIChip chip;
    CHECK(chip.isSimulated() == true);
}

TEST_CASE("SimSPIChip::beginTransaction and endTransaction are no-ops", "[spi]") {
    SimSPIChip chip;
    // These are no-ops; just verify they don't crash
    chip.beginTransaction(1000000);
    chip.endTransaction();
}
