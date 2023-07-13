#!/bin/sh
set -e
cd /backend
make vendor && make
