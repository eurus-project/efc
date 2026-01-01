# This file is part of the efc project <https://github.com/eurus-project/efc/>.
# Copyright (c) (2024 - Present), The efc developers.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(efc
    VERSION 0.1
    LANGUAGES C
)

target_sources(app
PRIVATE
    src/main.c
    src/radio_receiver.c
    src/telemetry_packer.c
    src/telemetry_sender.c
    src/logger.c
    src/motor_control.c
)

target_link_libraries(app
PRIVATE
    ulog
    mavlink
    esc
    mixer
)
