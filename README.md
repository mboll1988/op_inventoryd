# InventoryHIPA daemon

This repository contains a simple daemon applicaiton for the InventoryHIPA applicaiton ("Bestandesaufnahme") 
This repository also contains examples of starting scripts.

## Requirements

To build example of the daemon you have to have following tools

* CMake
* GCC/CLang

## Build

To build example of daemon you have to type following commands:

    git clone https://github.com/boll_m/op_inventoryd.git
    cd op_inventoryd
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr ../
    make
    sudo make install

## Usage

    systemctl start simple-daemon
    systemctl status simple-daemon
    systemctl reload simple-daemon
    systemctl stop simple-daemon

