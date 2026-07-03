#!/usr/bin/env bash
#
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
#
# Builds (if needed) and runs jMAVSim for EFC SITL.
#
# jMAVSim listens on the given UDP port and blocks until efc_sitl sends it
# the first packet, so start efc_sitl after (or concurrently with) this
# script -- jMAVSim will not send anything until it hears from efc_sitl.
#
# jMAVSim must be run with its own directory as the working directory, since
# it loads mavlink/message_definitions/common.xml via a relative path.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JMAVSIM_DIR="${SCRIPT_DIR}/../../tools/jMAVSim"
RUN_JAR="${JMAVSIM_DIR}/out/production/jmavsim_run.jar"

UDP_PORT="${EFC_SITL_JMAVSIM_PORT:-14560}"

if [ ! -f "${RUN_JAR}" ]; then
    echo "jmavsim_run.jar not found, building jMAVSim..."
    (cd "${JMAVSIM_DIR}" && ant create_run_jar copy_res)
fi

cd "${JMAVSIM_DIR}"

# Useful additional flags:
#   -automag                 look up the local magnetic field via NOAA instead of the Zurich default
#   -qgc <ip>:<port>         let jMAVSim bridge a ground station connection instead of efc_sitl doing it
#   -r <Hz>                  simulation refresh rate (default 250)
exec java -jar "${RUN_JAR}" -udp "${UDP_PORT}" "$@"
