// +-------------------------------------+
// | Initialization                      |
// |                                     |
// | Benigno Gobbo INFN Trieste          |
// | 20170619 V1.0                       |
// | 20170727 V1.1 Use JSON              |
// | 20170915 V1.1.1 Fixes               |
// +-------------------------------------+

#include <iostream>
#include <string>
// This is from here: https://github.com/nlohmann/json
#include "json.hpp"
// git cloned on 2017/07/27
#include "init.h"

using json = nlohmann::json;

Init*                Init::_init    = NULL;
std::vector<Bronko*> Init::_bronkos = { NULL, NULL, NULL, NULL };
std::vector<Vaisa*>  Init::_vaisas  = { NULL, NULL, NULL, NULL };
Adam*                Init::_adam    = NULL;

// These meed to be initialized...
std::bitset<4> snf::bgotvai( std::string( "0000" ) );
std::bitset<4> snf::bgotbro( std::string( "0000" ) );
std::bitset<8> snf::bgotgem( std::string( "00000000" ) );
// These don't (will be initialized from JSON file in Init)...
std::bitset<4> snf::busevai;
std::unordered_map<std::string, int> snf::vai;
std::string snf::usb485x4pn;
std::string snf::usb485x4sn;
std::bitset<4> snf::busebro;
std::unordered_map<std::string, int> snf::bro;
std::string snf::usb232x4pn;
std::string snf::usb232x4sn;
std::bitset<8> snf::busegem;
std::string snf::usb485cbpn;
std::string snf::usb485cbsn;

// <+><+><+><+><+><+> Single object instance
Init* Init::initialize( void ) {
  if( _init == 0 ) {
    _init = new Init;
    
  }
  return _init;
}

// <+><+><+><+><+><+> Constructor
Init::Init( void ) {

  // First of all set thinks from JSON file...
  std::ifstream f;
  f.open( snf::jsonFile );
  if( f.fail() ) {
    std::cout << "\033[31mOh shit!: cannot find " << snf::jsonFile << " file.\033[0m"
	      << std::endl;
    exit(1);
  }

  json j_devdata;
  f >> j_devdata;
  f.close();

  snf::busevai = std::bitset<4>( j_devdata.at("devdata").at("vaisala").at("busevai").get<std::string>() );
  snf::vai = j_devdata.at("devdata").at("vaisala").at("vai").get<std::unordered_map<std::string,int>>();
  snf::usb485x4pn = j_devdata.at("devdata").at("vaisala").at("usb485x4pn").get<std::string>();
  snf::usb485x4sn = j_devdata.at("devdata").at("vaisala").at("usb485x4sn").get<std::string>();
  snf::busebro = std::bitset<4>( j_devdata.at("devdata").at("bronkhorst").at("busebro").get<std::string>() );
  snf::bro = j_devdata.at("devdata").at("bronkhorst").at("bro").get<std::unordered_map<std::string,int>>();
  snf::usb232x4pn = j_devdata.at("devdata").at("bronkhorst").at("usb232x4pn").get<std::string>();
  snf::usb232x4sn = j_devdata.at("devdata").at("bronkhorst").at("usb232x4sn").get<std::string>();
  snf::busegem = std::bitset<8>( j_devdata.at("devdata").at("adam4019").at("busegem").get<std::string>() );
  snf::usb485cbpn = j_devdata.at("devdata").at("adam4019").at("usb485cbpn").get<std::string>();
  snf::usb485cbsn = j_devdata.at("devdata").at("adam4019").at("usb485cbsn").get<std::string>();
  // end of json stuff...
  
  std::cout << "\033[7mSearching for devices connected to USB ports. This may take a while...\033[0m"  << std::endl;  
  try {
    VaisaInit* vinit = VaisaInit::initialize(); 
    _vaisas = vinit->getVaisas();
  } catch( std::string error ) {
    throw( "\033[31mError during VAISALA initialisation: "+error+"\033[0m" );
  }

  try {
    BronkoInit* binit = BronkoInit::initialize(); 
    _bronkos = binit->getBronkos();
  } catch( std::string error ) {
    throw( "\033[31mError during BRONKHORST initialisation: "+error+"\033[0m" );
  }

  try {
    _adam = Adam::initialize();
    _adam->serialConnect();
  } catch( std::string error ) {
    throw( "\033[31mError during ADAM initialisation: "+error+"\033[0m" );
  }
  std::cout << "\033[7mEnd of USB ports scan.\033[0m"  << std::endl;  

}
