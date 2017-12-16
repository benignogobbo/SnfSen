// +-------------------------------------+
// | Vaisala DMT143 Dewpoint Transmitter |
// | serial wrapper                      |
// | Benigno Gobbo INFN Trieste          |
// | 20170620 V1.1                       |
// +-------------------------------------+

#include <iostream>
#include <string>
#include <sstream>
#include <typeinfo>
#include <cstring>
#include <cerrno>
#include <cmath>

#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include "vaisa.h"
#include "devdata.h"

VaisaInit* VaisaInit::_init = NULL;
std::vector<Vaisa*> VaisaInit::_vaisas = { NULL, NULL, NULL, NULL };

// <+><+><+><+><+><+> Single Object Instance;
VaisaInit* VaisaInit::initialize( void ) {
  if( _init == 0 ) {
    _init = new VaisaInit;
  }
  return _init;
}

// <+><+><+><+><+><+> Constructor
VaisaInit::VaisaInit( void ) {

  std::cout << "\033[4mLooking for " << snf::usb485x4pn << " (s/n "
	    << snf::usb485x4sn << ") module...\033[0m" << std::endl;
  char buff[256];
  std::string prod = "\"" + snf::usb485x4pn + "\"";
  std::string seri = "\"" + snf::usb485x4pn + "\"";
  std::string comm1 = "udevadm info -a -n ";
  std::string comm2 = " | xargs ";
  std::string comm3 = " | grep ";

  // Look for device:
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

  std::vector<std::string> foundDev;
  for( unsigned int i=0; i<devices.size(); i++ ) {
    bool found = false;
    std::string command = comm1 + devices[i] + comm2 + comm3 + prod + comm3 + seri;
    FILE* f = popen( command.c_str(), "r" );
    if( f ) {
      while( !feof( f ) ) {
        if( fgets( buff, 256, f ) != NULL && !found ) {
	  found = true;
          foundDev.push_back( devices[i] );
        }
      }
      pclose( f );
    }
  }

  
  if( foundDev.size() != 4 ) {
    std::cout << "\033[31m Found too few or many US-RS485 devices. They must be exactly FOUR. Exit...\033[0m" << std::endl;
    exit(1);
  }

  for( int i=0; i<4; i++ ) {
    bool alive = false;
    Vaisa* aVaisala = new Vaisa( foundDev[i] );
    try {
      alive = aVaisala->serialConnect();      
    } catch( std::string error ) {
      std::cout << "\033[31mError connecting to device " << foundDev[i] << ": "
		<< error << "\033[0m"  << std::endl;
      aVaisala->serialDisconnect();
      exit(1);
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
	throw( "Vaisala with s/n "+sn+" not in list" );
	return;
      }
    }
  }
  for( std::unordered_map<std::string, int>::const_iterator it=snf::vai.begin(); it!=snf::vai.end(); ++it ) {
    int j = it->second;
    if( snf::bgotvai[j] ) {
      std::cout << "\033[0mVaisala DM143, id: " << j << ", addr.: " << _vaisas[j]->getAddr()
		<< ", s/n: " << _vaisas[j]->getSerialNumber() << ", selected: "
		<< snf::yesno[snf::busevai.test(j)] << ", connected: " << snf::yesno[1] << "." << std::endl;
    }
    else {
      std::cout << "\033[0mVaisala DM143, id: " << j << ", addr.: -" 
		<< ", s/n: --------" << ", selected: "
		<< snf::yesno[snf::busevai.test(j)] << ", connected: " << snf::yesno[0] << "." << std::endl;
    }
  }
  
  return;
  
}

// <><><><><><> I needed this just for debugging...
void Vaisa::_dumpString( std::string s ) {
  for( int i=0; i<s.size(); i++ ) {
    char c = s.substr(i,1).c_str()[0];
    std::cout << std::hex << int(c) << std::dec;
    if( int(c)>31 && int(c)<127 ) std::cout << " " << c;
    std::cout << std::endl;
  }
}

// <><><><><><> Default Constructor
Vaisa::Vaisa( std::string device ) : _device(device), _fd(0) {
  _resetReadTimeout();
}

// <><><><><><> Scale readout timeout
const float _readTimeout0 = 0.12;
void Vaisa::_scaleReadTimeout( float scale ) {
  _readTimeout = _readTimeout0 * scale;
  return;
}

// <><><><><><> Reset readout timeout
void Vaisa::_resetReadTimeout( void ) {
  _readTimeout = _readTimeout0;
  return;
}

// <><><><><><> try to write some characters to serial
void Vaisa::_serialWrite( std::string data ) {
  int status = write( _fd, data.c_str(), data.size() );
  if( status != data.size() ) {
    throw( std::string( strerror( errno ) ) );
    return;
  }
}

// <><><><><><> try to read something from serial
std::string Vaisa::_serialRead() {

  fd_set read_fds, write_fds, except_fds;
  FD_ZERO( &read_fds );
  FD_ZERO( &write_fds );
  FD_ZERO( &except_fds );
  FD_SET( _fd, &read_fds );
  struct timeval timeout;

  timeout.tv_usec = modf( _readTimeout, &_readTimeout ) * 1000000; 
  timeout.tv_sec  = _readTimeout;
  _resetReadTimeout();
  
  std::string s = "";
  char buff[512];
  int status = 0;
  int rv = 0;
  do {
    if( ( rv = select( _fd+1, &read_fds, &write_fds, &except_fds, &timeout )) ==1 ) {
      status = read( _fd, buff, 1 );
      if( status == 1 ) {
	s += buff[0];
      }
    }
  } while( status > 0 && rv == 1 );

  return(s);
}

// <><><><><><> Get device address
int Vaisa::_getMyAddr( void ) {

  int addr = -1;
  try {
    std::string command = "addr\015"; // addr<cr>
    _serialWrite( command );
    _scaleReadTimeout( 3 );
    std::string answer = _serialRead();
    answer = answer.substr( command.size()+26,1 );
    addr = std::stoi( answer );
    usleep( 500000 );
    command = "\015"; 
    _serialWrite( command );
    _scaleReadTimeout( 3 );
    answer = _serialRead();
    if( addr < 1 || addr > 4 ) {
      throw( std::string( "Error during 'addr' command execution." ) );
      return(-1);
    } 
  } catch( std::string error ) {
    throw( error );
    return(-1);
  }
  return addr;
}

// <><><><><><> Get device serial number
std::string Vaisa::_getMySN( void ) {

  std::string answer = "";
  try {
    std::string command = "snum\015"; // addr<cr>
    _serialWrite( command );
    _scaleReadTimeout( 3 );
    answer = _serialRead();
    answer = answer.substr( command.size()+17,8 );
  } catch( std::string error ) {
    throw( error );
    return( std::string("") );
  }
  return answer;
}

// <><><><><><> Get the value (as string) specified by "value"
std::string Vaisa::_getValue( std::string value, std::string precision ) {

  std::string status = "", data = "";
  try {
    _scaleReadTimeout( 2 );
    std::string command = "form " + precision + "  " + value + " #000";  // form [string]
    status = sendCommand( command );
    if( status.substr(0,2) != "OK" ) {
      throw( std::string( "Error during 'form' command execution." ) );
      return(std::string(""));
    } 
    command = "send"; // send<cr>
    data = sendCommand( command );
    _scaleReadTimeout( 2 );
    command = "form /";  // form /<cr>
    status = sendCommand( command );
    if( status.substr(0,2) != "OK" ) {
      throw( std::string( "Error during 'form' command execution." ) );
      return(std::string(""));
    } 
  } catch( std::string error ) {
    throw( error );
    return(std::string(""));
  }
  size_t first = data.find_first_not_of(' ');
  size_t last = data.find_last_not_of(' ');
  return(data.substr(first, (last-first+1)));

}

// <><><><><><> Connect to Serial
bool Vaisa::serialConnect( void ) {

  _fd = open( _device.c_str(), O_RDWR | O_NOCTTY );
  if( _fd < 0 ) {
    throw( std::string( strerror( errno ) ) );
    return false;
  }  

  tcgetattr( _fd, &_oldtio );
  _newtio = _oldtio;

  cfsetispeed( &_newtio, (speed_t)B19200 );
  cfsetospeed( &_newtio, (speed_t)B19200 );
  cfmakeraw(   &_newtio );

  tcsetattr( _fd, TCSANOW, &_newtio );  
  tcflush( _fd, TCIOFLUSH );

  // A little bit tricky: I send a simple command to the "potential"
  // Vaisala and I check if I get a meningful answer...
  std::string snum = "snum\015";
  int status = write( _fd, snum.c_str(), snum.size() );
  usleep( 100000 );
  char buff[128];
  status = read( _fd, buff, sizeof( buff ) );
  status = strncmp( snum.c_str(), buff, status );
  if( status == 0 ) return false;

  _myAddr = _getMyAddr();
  _mySN   = _getMySN();
  
  return true;

}

// <><><><><><> Restore default Serial Settings and close it
void Vaisa::serialDisconnect( void ) {

  tcsetattr( _fd, TCSANOW, &_oldtio ); /* restore old port settings */
  tcflush( _fd, TCIOFLUSH );
  if( close( _fd ) < 0 ) {
    throw ( std::string( strerror( errno ) ) );
  } 

  return;

}

// <><><><><><> Ask for just a measurement
std::string Vaisa::getDataString( void ) {

  std::string data;
  try {
    std::string command = "form /";
    _scaleReadTimeout( 2 );
    std::string status = sendCommand( command );
    if( status.substr(0,2) != "OK" ) {
      throw( std::string( "Error during 'form' command execution." ) );
      return(std::string(""));
    }
    command = "send";
    data = sendCommand( command );
  } catch( std::string error ) {
    throw( error );
    return(std::string(""));
  }

  return(data);
}

// <><><><><><> Returns dew poit
float Vaisa::getTdf( int ndec ) {
  std::string prec = "3." + std::to_string(ndec);
  float val = 0;
  try {
    val = std::stof( _getValue( "tdf", prec ) );
  } catch( std::string error ) {
    throw( error );
    return(0.0);
  }
  return(val);
}

// <><><><><><> Returns dew poit normalized at 1 atm
float Vaisa::getTdfAtm( int ndec ) {
  std::string prec = "3." + std::to_string(ndec);
  float val = 0;
  try {
    val = std::stof( _getValue( "tdfa", prec ) );
  } catch( std::string error ) {
    throw( error );
    return(0.0);
  }
  return(val);
}

// <><><><><><> Returns vapour partial pressure in ppm
int   Vaisa::getH2O( void ) {
  int val = 0;
  try {
    val = std::stoi( _getValue( "H2O", "" ) );
  } catch( std::string error ) {
    throw( error );
    return(0.0);
  }
  return(val);
}

// <><><><><><> Send a command to the device
template <typename T> std::string Vaisa::sendCommand( std::string command, T argValue ) {

  if( ( command == "r" || command == "R" ) ) {
    throw( std::string( "Continuous outputting 'R' is not yet implemented." ) );
    return(std::string(""));
  }
  
  if( command == "?" ) _scaleReadTimeout( 4 );
  
  std::stringstream ss; ss << command << " " << argValue << "\015";

  try {
    _serialWrite( ss.str() );
  } catch( std::string error ) {
    throw( error );
    return(std::string(""));
  }
  
  std::string s = _serialRead();
  s = s.substr( ss.str().size() );
  return(s);
}

// <><><><><><> Send a command withour argument to the device
std::string Vaisa::sendCommand( std::string command ) {
  std::string s = "";
  try {
    s = sendCommand( command, std::string("") );
  } catch( std::string error ) {
    throw( error );
  }
  return(s);
}
