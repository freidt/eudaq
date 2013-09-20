#!/bin/bash
#
#
#

echo "Configuring DAQ for Single Event readout and TLU triggering"
${PATHSRS}/slow_control/tlu_triggering.py
${PATHSRS}/slow_control/prepare_single_acq_explorer1.py
echo ""
