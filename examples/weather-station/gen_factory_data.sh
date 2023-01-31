#!/bin/bash

set -e
cd $(dirname "$0")

for suffix in "" "_2" "_3"; do
  rm -rf build
  west build -b thingy53_nrf5340_cpuapp -t factory_data_hex -- -DOVERLAY_CONFIG="overlay-factory_data_build${suffix}.conf"

  mkdir -p factory_data${suffix}
  cp build/zephyr/{factory_data,CD,DAC,PAI}* factory_data${suffix}
done

