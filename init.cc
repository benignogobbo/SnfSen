// +--------------------------------------+
// | Initialization                       |
// | Benigno Gobbo INFN Trieste           |
// +--------------------------------------+

#include <iostream>
#include <string>

#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

// This is from here: https://github.com/nlohmann/json
#include "json.hpp"
// git cloned on 2017/07/27

#include "init.h"

using json = nlohmann::json;

Init*                Init::_init    = NULL;
std::vector<Bronko*> Init::_bronkos = { NULL, NULL, NULL, NULL };
std::vector<Vaisa*>  Init::_vaisas  = { NULL, NULL, NULL, NULL };
Adam*                Init::_adam    = NULL;

// These need to be initialized...
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

  try {
    // First of all set thinks from JSON file...
    readJSON();
    
    std::cout << "\033[7mSearching for devices connected to USB ports. This may take a while...\033[0m"  << std::endl;
    searchDevices();
  }
  catch( std::string error ) {
    throw( std::string( "\033[31mError during initialisation: " + error + "\033[0m" ) );
    return;
  }
}

// <+><+><+><+><+><+> readJSON
void Init::readJSON( void ) {

  std::ifstream f;
  f.open( snf::jsonFile );
  if( f.fail() ) {
    throw( std::string( "\033[31mOh shit!: cannot find " + snf::jsonFile + " file.\033[0m" ) );
    return;
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

  return;
}

// <+><+><+><+><+><+> searchDevices
void Init::searchDevices( void ) {

  // Get the name of all devices in /dev
  std::vector<std::string> files;
  DIR *dp = opendir( "/dev" );
  struct dirent *dirp;
  while( (dirp = readdir(dp))  != NULL ) {
    files.push_back( std::string( dirp->d_name ) );
  }
  std::string s = "ttyUSB";
  std::vector<std::string> devices;
  for( unsigned int i=0; i<files.size(); i++ ) {
    if( files[i].substr(0,6) == s ) {
      devices.push_back( "/dev/" + files[i] );
    }
  }

  if( snf::busevai.count() == 0 ) std::cout << "\033[33m Warning: no Vaisala devices set to be read.\033[0m" << std::endl;
  if( snf::busebro.count() == 0 ) std::cout << "\033[33m Warning: no Bronkhorst devices set to be read.\033[0m" << std::endl;
  if( snf::busegem.count() == 0 ) std::cout << "\033[33m Warning: no General Electric devices set to be read.\033[0m" << std::endl;

  std::cout << "\033[4mLooking for:\033[0m" << std::endl;
  if( snf::busevai.count() > 0 ) std::cout << snf::usb485x4pn << " (s/n " << snf::usb485x4sn << ") module..." << std::endl;
  if( snf::busebro.count() > 0 ) std::cout << snf::usb232x4pn << " (s/n " << snf::usb232x4sn << ") module..." << std::endl;
  if( snf::busegem.count() > 0 ) std::cout << snf::usb485cbpn << " (s/n " << snf::usb485cbsn << ") module..." << std::endl;
  
  char buff[256];

  std::vector<std::string> vFoundDev, bFoundDev, aFoundDev;
  FILE* f;
  for( unsigned int i=0; i<devices.size(); i++ ) {

    bool vFound = false, bFound = false, aFound = false;
    
    if( snf::busevai.count() > 0 ) {
      std::string vcomm = "udevadm info -a -n " + devices[i] + " | xargs | awk '/" + snf::usb485x4pn + "/ && /" + snf::usb485x4sn + "/'";
      f = popen( vcomm.c_str(), "r" );
      if( f ) {
	while( !feof( f ) ) {
	  if( fgets( buff, 256, f ) != NULL && !vFound ) {
	    vFound = true;
	    vFoundDev.push_back( devices[i] );
	  }
	}
	pclose( f );
      }
    }

    if( snf::busebro.count() > 0 ) {
      std::string bcomm = "udevadm info -a -n " + devices[i] + " | xargs | awk '/" + snf::usb232x4pn + "/ && /" + snf::usb232x4sn + "/'";
      f = popen( bcomm.c_str(), "r" );
      if( f ) {
	while( !feof( f ) ) {
	  if( fgets( buff, 256, f ) != NULL && !bFound ) {
	    bFound = true;
	    bFoundDev.push_back( devices[i] );
	  }
	}
	pclose( f );
      }
    }

    if( snf::busegem.count() > 0 ) {
      std::string acomm = "udevadm info -a -n " + devices[i] + " | xargs | awk '/" + snf::usb485cbpn + "/ && /" + snf::usb485cbsn + "/'";
      f = popen( acomm.c_str(), "r" );
      if( f ) {
	while( !feof( f ) ) {
	  if( fgets( buff, 256, f ) != NULL && !aFound ) {
	    aFound = true;
	  aFoundDev.push_back( devices[i] );
	  }
	}
	pclose( f );
      }
    }
  }

  if( snf::busevai.count() > 0 && vFoundDev.size() != 4 ) {
    throw( std::string( "\033[31m Found too few or many 'USB-COM485 Plus4' devices. They must be exactly FOUR. Exit...\033[0m" ) );
    return;
  }

  
  if( snf::busebro.count() > 0 && bFoundDev.size() != 4 ) {
    throw( std::string( "\033[31m Found too few or many 'USB-COM232 Plus4' devices. They must be exactly FOUR. Exit...\033[0m" ) );
    return;
  }
  
  if( snf::busegem.count() > 0 && aFoundDev.size() != 1 ) {
    throw( std::string( "\033[31m Found too few or many 'USB-RS485 Cable' devices. It must be exactly ONE. Exit...\033[0m" ) );
    return;
  }

  if( snf::busevai.count() > 0 ) {
    for( int i=0; i<4; i++ ) {
      bool alive = false;
      Vaisa* aVaisala = new Vaisa( vFoundDev[i] );
      try {
	alive = aVaisala->serialConnect();      
      } catch( std::string error ) {
	throw( std::string( "\033[31mError connecting to device " + vFoundDev[i] + ": " + error + "\033[0m" ) );
	return;
      }
      if( alive ) {
	std::string sn = aVaisala->getSerialNumber();
	std::unordered_map<std::string,int>::const_iterator gotIt = snf::vai.find( sn ); 
	if( gotIt != snf::vai.end() ) {
	  int j = gotIt->second;
	  if( snf::busevai.test( j ) ) {
	    _vaisas[ j ] = aVaisala;
	    snf::bgotvai.set( j, 1 );
	  }
	}
	else {
	  throw( std::string( "Vaisala with s/n "+sn+" not in list" ) );
	  return;
	}
      }
    }
    for( std::unordered_map<std::string, int>::const_iterator it=snf::vai.begin(); it!=snf::vai.end(); ++it ) {
      int j = it->second;
      if( snf::bgotvai[j] ) {
	std::cout << "\033[0mVaisala " << _vaisas[j]->getProductType() << ", id: " << j << ", addr.: " << _vaisas[j]->getAddr()
		  << ", s/n: " << _vaisas[j]->getSerialNumber() << ", selected: "
		  << snf::yesno[snf::busevai.test(j)] << ", connected: " << snf::yesno[1] << "." << std::endl;
      }
      else {
	std::cout << "\033[0mVaisala DMT1xx, id: " << j << ", addr.: -" 
		  << ", s/n: --------" << ", selected: "
		  << snf::yesno[snf::busevai.test(j)] << ", connected: " << snf::yesno[0] << "." << std::endl;
      }
    }
  }

  if( snf::busebro.count() > 0 ) {
    for( int i=0; i<4; i++ ) {
      bool alive = false;
      Bronko* aBronkhorst = new Bronko( bFoundDev[i] );
      try {
	alive = aBronkhorst->serialConnect();      
      } catch( std::string error ) {
	throw( std::string( "\033[31mError connecting to device " + bFoundDev[i] + ": " + error + "\033[0m" ) );
	return;
      }
      if( alive ) {
	std::string sn = aBronkhorst->getSerialNumber();
	std::unordered_map<std::string,int>::const_iterator gotIt = snf::bro.find( sn ); 
	if( gotIt != snf::bro.end() ) {
	  int j = gotIt->second;
	  if( snf::busebro.test( j ) ) {
	    _bronkos[ j ] = aBronkhorst;
	    snf::bgotbro.set( j, 1 );		  
	  }
	}
	else {
	  throw( std::string( "Bronkhorst with s/n "+sn+" not in list" ) );
	  return;
	}
      }
    }
    for( std::unordered_map<std::string, int>::const_iterator it=snf::bro.begin(); it!=snf::bro.end(); ++it ) {
      int j = it->second;
      if( snf::bgotbro[j] ) {
	std::cout << "\033[0mBronkHorst F-101E-AGD-33-V, id: " << j
		  << ", s/n: " << _bronkos[j]->getSerialNumber() << ", selected: "
		  << snf::yesno[snf::busebro.test(j)] << ", connected: " << snf::yesno[1] << "." << std::endl;
      }
      else {
	std::cout << "\033[0mBronkHorst F-101E-AGD-33-V, id: " << j
		  << ", s/n: ----------" << ", selected: "
		  << snf::yesno[snf::busebro.test(j)] << ", connected: " << snf::yesno[0] << "." << std::endl;
      }
    }
  }

  if( snf::busegem.count() > 0 ) {
    bool alive = false;
    Adam* anAdam = new Adam( aFoundDev[0] );
    try {
      alive = anAdam->serialConnect();      
    } catch( std::string error ) {
      throw( std::string( "\033[31mError connecting to device " + aFoundDev[0] + ": " + error + "\033[0m" ) );
      return;
    }
    _adam = anAdam;
  }
  
}

// <+><+><+><+><+><+> cleanDevices
void Init::cleanDevices( void ) {

  try {
    for( int i=0; i<4; i++ ) {
      _vaisas[i]->serialDisconnect();
      delete( _vaisas[i] );
      _vaisas[i] = NULL;
    }
  }  catch( std::string error ) {
    throw( std::string( "\033[31mError disconnecting and deleting a Vaisa object: " + error + "\033[0m" ) );
    return;
  }

  try {
    for( int i=0; i<4; i++ ) {
      _bronkos[i]->serialDisconnect();
      delete( _bronkos[i] );
      _bronkos[i] = NULL;
    }
  }  catch( std::string error ) {
    throw( std::string( "\033[31mError disconnecting and deleting a Bronko object: " + error + "\033[0m" ) );
    return;
  }

  try {
    _adam->serialDisconnect();
    delete( _adam );
    _adam = NULL;
  }  catch( std::string error ) {
    throw( std::string( "\033[31mError disconnecting and deleting the Adam object: " + error + "\033[0m" ) );
    return;
  }
  
}

void Init::reconnectDevices( void ) {
  try {
    cleanDevices();
    searchDevices();
  }  catch( std::string error ) {
    throw( std::string( "\033[31mError disconnecting and reconnecting the devices: " + error + "\033[0m" ) );
    return;
  }

}
