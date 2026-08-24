/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) (2024 - Present), The efc developers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* Host-side unit tests for the mixer library; not part of the Zephyr build. */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "mixer.h"

namespace {

MIXER_Inst_Type quad_x_mixer() {
    MIXER_Inst_Type mixer;
    REQUIRE(MIXER_Init(&mixer, MIXER_FRAME_QUAD_X) == MIXER_SUCCESS);
    return mixer;
}

float output_mean(const MIXER_Output_Type &output, uint8_t motor_count) {
    float sum = 0.0f;

    for (uint8_t i = 0; i < motor_count; i++) {
        sum += output.motor[i];
    }

    return sum / static_cast<float>(motor_count);
}

/* MIXER_Calculate's [0, 1] guarantee is exact in real arithmetic. In float,
   the torque scale (MIXER_TORQUE_HEADROOM / max_torque_demand) is generally
   not exactly representable, so a boundary output can round out of range by an
   ULP or two. */
constexpr float kBoundTolerance = 1e-6f;

} // namespace

TEST_CASE("MIXER_Init rejects a null instance and an unsupported frame") {
    MIXER_Inst_Type mixer;

    CHECK(MIXER_Init(nullptr, MIXER_FRAME_QUAD_X) == MIXER_INVALID_ARG);
    CHECK(MIXER_Init(&mixer, (MIXER_Frame_Type)42) == MIXER_INVALID_CFG);
    CHECK(MIXER_Init(&mixer, MIXER_FRAME_QUAD_X) == MIXER_SUCCESS);
}

TEST_CASE("MIXER_Calculate validates arguments and initialization state") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    const MIXER_Input_Type input = {0.0f, 0.0f, 0.0f, 0.0f};
    MIXER_Output_Type output;

    CHECK(MIXER_Calculate(nullptr, &input, &output) == MIXER_INVALID_ARG);
    CHECK(MIXER_Calculate(&mixer, nullptr, &output) == MIXER_INVALID_ARG);
    CHECK(MIXER_Calculate(&mixer, &input, nullptr) == MIXER_INVALID_ARG);

    /* An instance that was never initialized has no geometry bound */
    const MIXER_Inst_Type uninitialized = {nullptr, 0.0f};
    CHECK(MIXER_Calculate(&uninitialized, &input, &output) ==
          MIXER_INVALID_CFG);
}

TEST_CASE("collective thrust alone passes straight through") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    const MIXER_Input_Type input = {0.5f, 0.0f, 0.0f, 0.0f};
    MIXER_Output_Type output = {};

    REQUIRE(MIXER_Calculate(&mixer, &input, &output) == MIXER_SUCCESS);
    for (uint8_t i = 0; i < mixer.geometry->motor_count; i++) {
        CHECK(output.motor[i] == doctest::Approx(0.5f));
    }
}

TEST_CASE(
    "zero demand does not stop the motors: idle sits at the headroom floor") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    const MIXER_Input_Type input = {0.0f, 0.0f, 0.0f, 0.0f};
    MIXER_Output_Type output = {};

    REQUIRE(MIXER_Calculate(&mixer, &input, &output) == MIXER_SUCCESS);
    /* Unlike the previous dynamic mixer, the headroom reservation is
       unconditional: idle throttle still costs MIXER_TORQUE_HEADROOM. */
    for (uint8_t i = 0; i < mixer.geometry->motor_count; i++) {
        CHECK(output.motor[i] == doctest::Approx(MIXER_TORQUE_HEADROOM));
    }
}

TEST_CASE("full roll splits the frame symmetrically around mid throttle") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    const MIXER_Input_Type input = {0.5f, 1.0f, 0.0f, 0.0f};
    MIXER_Output_Type output = {};

    REQUIRE(MIXER_Calculate(&mixer, &input, &output) == MIXER_SUCCESS);
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_RIGHT] ==
          doctest::Approx(0.357143f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_RIGHT] ==
          doctest::Approx(0.357143f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_LEFT] ==
          doctest::Approx(0.642857f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_LEFT] ==
          doctest::Approx(0.642857f));
}

TEST_CASE("yaw authority is an order of magnitude below roll/pitch") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    const MIXER_Input_Type roll_in = {0.5f, 1.0f, 0.0f, 0.0f};
    const MIXER_Input_Type yaw_in = {0.5f, 0.0f, 0.0f, 1.0f};
    MIXER_Output_Type roll_out = {};
    MIXER_Output_Type yaw_out = {};

    REQUIRE(MIXER_Calculate(&mixer, &roll_in, &roll_out) == MIXER_SUCCESS);
    REQUIRE(MIXER_Calculate(&mixer, &yaw_in, &yaw_out) == MIXER_SUCCESS);

    const float roll_swing =
        roll_out.motor[MIXER_QUAD_X_MOTOR_FRONT_LEFT] - 0.5f;
    const float yaw_swing =
        yaw_out.motor[MIXER_QUAD_X_MOTOR_FRONT_RIGHT] - 0.5f;
    CHECK(roll_swing / yaw_swing == doctest::Approx(10.0f));
}

TEST_CASE("mixes all axes onto the motors, yaw at reduced authority") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    /* Same input the Python simulator's bridge test uses, so both suites
       cross-check the same numbers. Yaw contributes an order of magnitude
       less than roll/pitch by design, so the mix deliberately does not
       preserve the requested roll/pitch/yaw ratio. */
    const MIXER_Input_Type input = {0.5f, 0.4f, -0.2f, 0.6f};
    MIXER_Output_Type output = {};

    REQUIRE(MIXER_Calculate(&mixer, &input, &output) == MIXER_SUCCESS);
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_RIGHT] ==
          doctest::Approx(0.480000f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_LEFT] ==
          doctest::Approx(0.537143f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_LEFT] ==
          doctest::Approx(0.577143f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_RIGHT] ==
          doctest::Approx(0.405714f));
    CHECK(output_mean(output, mixer.geometry->motor_count) ==
          doctest::Approx(0.5f));
}

TEST_CASE(
    "worst-case demand lands exactly on the upper bound, never above it") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    /* Thrust and all three axes maxed and aligned on the same motor: the
       exact worst case MIXER_TORQUE_HEADROOM was derived from. */
    const MIXER_Input_Type input = {1.0f, 1.0f, 1.0f, 1.0f};
    MIXER_Output_Type output = {};

    REQUIRE(MIXER_Calculate(&mixer, &input, &output) == MIXER_SUCCESS);
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_RIGHT] ==
          doctest::Approx(0.428571f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_LEFT] == doctest::Approx(1.0f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_LEFT] ==
          doctest::Approx(0.685714f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_RIGHT] ==
          doctest::Approx(0.685714f));
    for (uint8_t i = 0; i < mixer.geometry->motor_count; i++) {
        CHECK(output.motor[i] <= 1.0f + kBoundTolerance);
    }
}

TEST_CASE(
    "worst-case demand lands exactly on the lower bound, never below it") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    const MIXER_Input_Type input = {0.0f, -1.0f, -1.0f, -1.0f};
    MIXER_Output_Type output = {};

    REQUIRE(MIXER_Calculate(&mixer, &input, &output) == MIXER_SUCCESS);
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_RIGHT] ==
          doctest::Approx(0.571429f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_LEFT] == doctest::Approx(0.0f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_LEFT] ==
          doctest::Approx(0.314286f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_RIGHT] ==
          doctest::Approx(0.314286f));
    for (uint8_t i = 0; i < mixer.geometry->motor_count; i++) {
        CHECK(output.motor[i] >= -kBoundTolerance);
    }
}

TEST_CASE("outputs stay within [0, 1] across the entire nominal input range") {
    MIXER_Inst_Type mixer = quad_x_mixer();

    for (float thrust = 0.0f; thrust <= 1.0f; thrust += 0.1f) {
        for (float roll = -1.0f; roll <= 1.0f; roll += 0.25f) {
            for (float pitch = -1.0f; pitch <= 1.0f; pitch += 0.25f) {
                for (float yaw = -1.0f; yaw <= 1.0f; yaw += 0.25f) {
                    const MIXER_Input_Type input = {thrust, roll, pitch, yaw};
                    MIXER_Output_Type output = {};

                    REQUIRE(MIXER_Calculate(&mixer, &input, &output) ==
                            MIXER_SUCCESS);
                    for (uint8_t i = 0; i < mixer.geometry->motor_count; i++) {
                        CHECK(output.motor[i] >= -kBoundTolerance);
                        CHECK(output.motor[i] <= 1.0f + kBoundTolerance);
                    }
                }
            }
        }
    }
}

TEST_CASE("demands outside the nominal range are not clamped by the mixer") {
    /* Intentional, not an oversight: MIXER_Calculate no longer tolerates
       out-of-range input the way the previous dynamic implementation did
       (see the MIXER_Input_Type contract in mixer.h). This test documents
       that boundary so it isn't "fixed" by accident later. */
    MIXER_Inst_Type mixer = quad_x_mixer();
    const MIXER_Input_Type input = {0.5f, 8.0f, 0.0f, 0.0f};
    MIXER_Output_Type output = {};

    REQUIRE(MIXER_Calculate(&mixer, &input, &output) == MIXER_SUCCESS);
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_REAR_LEFT] ==
          doctest::Approx(1.642857f));
    CHECK(output.motor[MIXER_QUAD_X_MOTOR_FRONT_RIGHT] ==
          doctest::Approx(-0.642857f));
}

TEST_CASE("input is not mutated") {
    MIXER_Inst_Type mixer = quad_x_mixer();
    const MIXER_Input_Type input = {0.9f, 0.9f, 0.3f, 0.3f};
    const MIXER_Input_Type copy = input;
    MIXER_Output_Type output;

    REQUIRE(MIXER_Calculate(&mixer, &input, &output) == MIXER_SUCCESS);

    CHECK(input.thrust == copy.thrust);
    CHECK(input.roll == copy.roll);
    CHECK(input.pitch == copy.pitch);
    CHECK(input.yaw == copy.yaw);
}
