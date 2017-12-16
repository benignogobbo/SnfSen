// +-------------------------------------+
// | ADAM-4019+                          |
// | serial wrapper                      |
// | Benigno Gobbo INFN Trieste          |
// | 20170620 V1.2                       |
// +-------------------------------------+

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <typeinfo>
#include <cstring>
#include <cerrno>
#include <cmath>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "devdata.h"
#include "adam.h"

Adam* Adam::_init = 0;

// <><><><><><> Single Object Instance;
Adam* Adam::initialize( void ) {
  if( _init == 0 ) {
    _init = new Adam;
  }
  return _init;
}

// <><><><><><> Default Constructor
Adam::Adam( void ) : _fd(0) {
  _resetReadTimeout();
}

// <><><><><><> Scale readout timeout
const float _readTimeout0 = 0.12;
void Adam::_scaleReadTimeout( float scale ) {
  _readTimeout = _readTimeout0 * scale;
  return;
}

// <><><><><><> Reset readout timeout
void Adam::_resetReadTimeout( void ) {
  _readTimeout = _readTimeout0;
  return;
}

// <><><><><><> try to write some characters to serial
void Adam::_serialWrite( std::string data ) {
  int status = write( _fd, data.c_str(), data.size() );
  if( status != data.size() ) {
    throw( std::string( strerror( errno ) ) );
    return;
  }
}

// <><><><><><> try to read something from seriar (slow but safer...)
std::string Adam::_serialRead() {

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

// <><><><><><> try to read something from serial, fast but unsafe
std::string Adam::_serialFastRead() {
  char buff[256];
  int status = read( _fd, buff, sizeof(buff) );
  if( status < 0 ) {
    throw( std::string( strerror( errno ) ) );
    return( std::string( "" ) );
  }
  else {
    return( std::string( buff ) );
  }
}

// <><><><><><> Connect to Serial
void Adam::serialConnect( void ) {

  std::cout << "\033[4mLooking for " << snf::usb485cbpn << " (s/n "
	    << snf::usb485cbsn << ") module... \033[0m" << std::endl;
  char buff[256];
  std::string prod = "\"" + snf::usb485cbpn + "\"";
  std::string seri = "\"" + snf::usb485cbpn + "\"";
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
  
  if( foundDev.size() != 1 ) {
    std::cout << "\033[31m Found too few or many US-RS485 devices. I need just one. Exit...\033[0m" << std::endl;
    exit(1);
  }
  

  _fd = open( foundDev[0].c_str(), O_RDWR | O_NOCTTY );
  if( _fd < 0 ) {
    throw( std::string( strerror( errno ) ) );
    return;
  }  

  tcgetattr( _fd, &_oldtio );
  _newtio = _oldtio;

  cfsetispeed( &_newtio, (speed_t)B9600 );
  cfsetospeed( &_newtio, (speed_t)B9600 );
  cfmakeraw(   &_newtio );

  tcsetattr( _fd, TCSANOW, &_newtio );  
  tcflush( _fd, TCIOFLUSH );

  // Look now for the first responding ADAM-4019+ device...
  bool found = false;
  for( int i=0; !found && i<255; i++ ) {
    std::stringstream ss; ss << std::setfill('0') << std::setw(2) << std::hex << i;
    std::string command = "$" + ss.str() + "M\r";
    _serialWrite( command );
    usleep( 50000 );
    std::string devName = _serialRead();
    if( devName.size() > 7 ) {
      if( devName.substr(0,1) == "!" && devName.substr(3,5) == "4019P" ) {
	found = true;
	_myAddr = ss.str();
      }
    }
  }  
  if( !found ) {
    throw( "No ADAM-4019+ devices found..." );
    return;
  }

  std::cout << "\033[0mConnected to an \033[0mADAM-4019+\033[0m device, address " << _myAddr << "." << std::endl;

  // Look at analog input with something connected...
  for( int i=0; i<8; i++ ) {
    if( snf::busegem.test(i) ) {
      std::stringstream ss; ss << "#" << _myAddr << std::hex << i << "\r"; 
      _serialWrite( ss.str() );
      usleep( 100000 );
      std::string res = _serialRead();
      if( res.substr(0,8) != ">+888888" ) {
	snf::bgotgem.set( i, 1 );
      }
    }
    std::cout << "\033[0mAnalog addr. " << i << ", selected: " << snf::yesno[snf::busegem.test(i)]
		<< ", connected: " << snf::yesno[snf::bgotgem.test(i)] << "." << std::endl;
  }
  
  return;

}

// <><><><><><> Restore default Serial Settings and close it
void Adam::serialDisconnect( void ) {

  tcsetattr( _fd, TCSANOW, &_oldtio ); /* restore old port settings */
  tcflush( _fd, TCIOFLUSH );
  if( close( _fd ) < 0 ) {
    throw ( std::string( strerror( errno ) ) );
  } 
  return;
}

// <><><><><><> Commands...
// <><><><><><>
std::string Adam::_checkChannel( int channel ) {

  if( channel < 0 || channel > 7 ) {
    return( "ADAM-4019+ channel should be between 0 and 7" );
  }
  else if( !snf::busegem.test( channel ) ) {
    return( "ADAM-4019+ channel " + std::to_string(channel) + " is set as non active" );
  }
  else if( !snf::bgotgem.test( channel ) ) {
    return( "ADAM-4019+ channel " + std::to_string(channel) + " is non active" );
  }
  else {
    return( "" );
  }
}

// <><><><><><>
double Adam::getMeasurement ( int channel ) {

  std::string cok = _checkChannel( channel );
  if( cok != "" ) {
    throw( cok );
    return 0;
  }
  
  std::stringstream ss;
  ss << "#"<< _myAddr << channel << "\r";
  _serialWrite( ss.str() );
  usleep( 100000 );
  std::string res = _serialRead();
  if( res.size() > 0 ) {
    if( res.substr(0,1) == "?" ) {
      throw( "Invalid request sent to "+_myAddr );
      return 0;
    }
    else if( res.substr(0,1) == ">" ) {
      return( std::stod( res.substr(1) ) );
    }
    else {
      throw( "Meaningless answer from "+_myAddr );
      return 0;
    }
  }
  else {
    throw( "Empty answer from "+_myAddr );
    return 0;
  }  
  return 0;
}
