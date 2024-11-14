# InventoryHIPA daemon

This repository contains a simple daemon applicaiton for the InventoryHIPA applicaiton ("Bestandesaufnahme") 
This repository also contains examples of starting scripts.

## Requirements

To build you need following tools

* CMake
* GCC/CLang

## Build

Type following commands:

    git clone https://github.com/mboll1988/op_inventoryd.git
    cd op_inventoryd
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr ../
    make
    sudo make install

## Usage

    systemctl start op_inventoryd.service
    systemctl status op_inventoryd.service
    systemctl reload op_inventoryd.service
    systemctl stop op_inventoryd.service

