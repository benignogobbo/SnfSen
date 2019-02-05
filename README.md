# SnfSen

A library was developed to allow the Belle2 frost point sniffers sensors readout using a Linux machine. It does not need additional system libraries, but it uses the udevadm system command. This command is in general installed by default in most common Linux Distributions, and in any case it belongs from udev package in Debian-like distributions and from systemd package in RedHat-like ones. A gcc compiler version that supports C++11 standard is needed to build the software.

The software is available as an xzip-ed tar archive. Unpacking the archive a ./snfsen directory will be create. Go to that directory and type make to build the software. Two test examples called test and loop should be available after software build. test  tries to access to all sensors and to make a single data readout; loop reads and displays sensors data continuously. To run test examples one needs of course to connect the Linux computer to the sniffer crate via an USB port.

NOTE: a JSON file (devdata.json) contains all information needed by the program to access the interfaces to the sensors. Any change in hardware -e.g. a broken component replacement- needs a reconfiguration of that file. Some description of the quantities contained in that file can be found it devdata.h file.

NOTE: json.hpp supports gcc compilers from version 4.9. See https://github.com/nlohmann/json#supported-compilers, we should pay attention on limitations as we use version 4.8

Support has been extented to DMT152 Vaisala Dew Point Transmitters
