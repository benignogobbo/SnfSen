#ifndef DEVDATA_H
#define DEVDATA_H

// This file contains all data neede to univocally identify 
// devices connected to USB ports. Its quantities are
// initialized from devdata.json file by Init class.

#include <unordered_map>
#include <string>
#include <bitset>
#include <fstream>

namespace snf {

  // This file must contain all devices info
  const std::string jsonFile = "devdata.json";
  
  // Dew/Frost point transmitters
  // This bitset allows to (un)select some devices 
  extern std::bitset<4> busevai;
  // This bitsel flags the found devices
  extern std::bitset<4> bgotvai;
  //
  // Vaisala DMT143 serial <-> id association. NOTE addr = id+1
  extern std::unordered_map<std::string, int> vai;
  //
  // FTDI USB-COM485-PLUS4
  extern std::string usb485x4pn;
  extern std::string usb485x4sn;
  
  // Flowmeters
  // This bitset allows to (un)select some devices 
  extern std::bitset<4> busebro;
  // This bitsel flags the found devices
  extern std::bitset<4> bgotbro;
  //
  // Bronkhorst F-101E-AGD-33-V
  extern std::unordered_map<std::string, int> bro;
  //
  // FTDI USB-COM232-PLUS4
  extern std::string usb232x4pn;
  extern std::string usb232x4sn;

  // Pressure transmitters (GEMS 3500B0285A01B000)
  //
  // This bitset allows to (un)select some devices 
  extern std::bitset<8> busegem;
  // This bitsel flags the found devices
  extern std::bitset<8> bgotgem;
  //
  // ADAM-4019+ USB-RS485 Cable
  extern std::string usb485cbpn;
  extern std::string usb485cbsn;

  const std::string yesno[2] = { " no", "YES" };
}

#endif // DEVDATA_H
