# Performing Device Firmware Upgrade in nRF Connect SDK examples

The following examples for the development kits from Nordic Semiconductor
support over-the-air Device Firmware Upgrade:

-   [CHIP nRF Connect Lock Example Application](../../examples/lock-app/nrfconnect/README.md)
-   [CHIP nRF Connect Lighting Example Application](../../examples/lighting-app/nrfconnect/README.md)

The
[CHIP nRF Connect Pigweed Example Application](../../examples/pigweed-app/nrfconnect/README.md)
does not support DFU.

Currently the Bluetooth LE is the only available transport for performing Device Firmware Upgrade operation and for that purpose it uses [Simple Management Protocol](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/guides/device_mgmt/index.html#device-mgmt). Upgrade can be done either using smartphone application or PC command line tool.

## Device Firmware Upgrade using smartphone

To upgrade your device firmware over Bluetooth LE using smartphone, complete the following tasks:

1. Install on your smartphone [nRF Connect for Mobile](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Connect-for-mobile) or [nRF Toolbox](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Toolbox) application.
2. Push **Button 4** on the device to start Bluetooth LE advertising.
3. Push **Button 1** on the device to enable software update functionality.
4. Follow the instructions in the section about downloading the new image to a device on the
[FOTA upgrades](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/ug_nrf52.html#fota-upgrades)
page in the nRF Connect documentation.

## Device Firmware Upgrade using PC command line tool

To upgrade your device firmware over Bluetooth LE, you can use PC command line tool provided by the [mcumgr](https://github.com/zephyrproject-rtos/mcumgr) project.

> **_WARNING:_** mcumgr tool using Bluetooth LE is available only for Linux and macOS systems.
> On Windows there is no support for Device Firmware Upgrade over BLE yet.

Complete the following steps to perform DFU using mcumgr:

1. Follow the [mcumgr command line tool installation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/guides/device_mgmt/index.html#command-line-tool) instruction to install the tool.
2. Push **Button 4** on the device to start Bluetooth LE advertising.
3. Push **Button 1** on the device to enable software update functionality.
4. Navigate to your example directory and call the following command:

        $ cd build/zephyr

> **_NOTE:_** In all of the commands listed in the following steps replace `ble-controller-name` with the Bluetooth controller name (e.g. hci0) and `ble-device-name` with the CHIP device name advertised over Bluetooth LE (e.g.ChipLock)
>

5. Upload firmware image to the device, by invoking following command:

        $ sudo mcumgr --conntype ble --connstring ctlr_name=ble-controller-name,peer_name='ble-device-name' image upload app_update.bin

    That operation may take few minutes and should be finished when progress bar will reach 100%, as presented below:

        565.25 KiB / 565.25 KiB [========================================================================] 100.00% 2.07 KiB/s 4m32s
        Done

6. Obtain a list of images present in the device memory, by invoking following command:

        $ sudo mcumgr --conntype ble --connstring ctlr_name=ble-controller-name,peer_name='ble-device-name' image list

    The displayed output contain the old image in slot 0 that is currently active and the new image in slot 1, which is not active yet (flags field empty):

        Images:
        image=0 slot=0
            version: 0.0.0
            bootable: true
            flags: active confirmed
            hash: 7bb0e909a846e833465cbb44c581cf045413a5446c6953a30a3dcc2c3ad51764
        image=0 slot=1
            version: 0.0.0
            bootable: true
            flags: 
            hash: cbd58fc3821e749d3abfb00b3069f98c078824735f1b2a333e8a1579971e7de1
        Split status: N/A (0)

7. Swap firmware images, by calling the following method with `image-hash` replaced by the image present in the slot 1 hash (e.g.cbd58fc3821e749d3abfb00b3069f98c078824735f1b2a333e8a1579971e7de1):

        $ sudo mcumgr --conntype ble --connstring ctlr_name=ble-controller-name,peer_name='ble-device-name' image test image-hash

    You can observe that in image present in slot 1 `flags:` field should change value to `pending`:

        Images:
        image=0 slot=0
            version: 0.0.0
            bootable: true
            flags: active confirmed
            hash: 7bb0e909a846e833465cbb44c581cf045413a5446c6953a30a3dcc2c3ad51764
        image=0 slot=1
            version: 0.0.0
            bootable: true
            flags: pending
            hash: cbd58fc3821e749d3abfb00b3069f98c078824735f1b2a333e8a1579971e7de1
        Split status: N/A (0)

8. Reset the device with the following command to let bootloader swap images:

        $ sudo mcumgr --conntype ble --connstring ctlr_name=ble-controller-name,peer_name='ble-device-name' reset

    You should be able to see that device was reset and the following logs appeared on its console:

        *** Booting Zephyr OS build zephyr-v2.5.0-1101-ga9d3aef65424  ***
        I: Starting bootloader
        I: Primary image: magic=good, swap_type=0x2, copy_done=0x1, image_ok=0x1
        I: Secondary image: magic=good, swap_type=0x2, copy_done=0x3, image_ok=0x3
        I: Boot source: none
        I: Swap type: test

    Swapping operation may take some time, but after that new firmware will be booted. Please note that first firmware boot is the test boot to check integrity and allow for image revert in case of problems, but after another hardware reset firmware will be booted in the normal way.
 
Visit the [mcumgr image management](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/guides/device_mgmt/indexhtml#image-management) section to familiarize with all image management commands supported by the tool.
