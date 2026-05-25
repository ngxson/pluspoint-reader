#!/bin/bash
set -e
pio run -e native
.pio/build/native/program
