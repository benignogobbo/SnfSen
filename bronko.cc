// +-------------------------------------+
// | Bronkhorst flow meter               |
// | serial wrapper                      |
// | Benigno Gobbo INFN Trieste          |
// | 20170620 V1.1                       |
// +-------------------------------------+

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <typeinfo>
#include <cstring>
#include <cerrno>
#include <cmath>

#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include "bronko.h"
#include "devdata.h"

BronkoInit* BronkoInit::_init = NULL;
std::vector<Bronko*> BronkoInit::_bronkos = { NULL, NULL, NULL, NULL };

// <+><+><+><+><+><+> Single Object Instance;
BronkoInit* BronkoInit::initialize( void ) {
  if( _init == 0 ) {
    _init = new BronkoInit;
  }
  return _init;
}

// <+><+><+><+><+><+> Constructor
BronkoInit::BronkoInit( void ) {

  std::cout << "\033[4mLooking for " << snf::usb232x4pn << " (s/n "
	    << snf::usb232x4sn << ") module...\033[0m" << std::endl;
  char buff[256];
  std::string prod = "\"" + snf::usb232x4pn + "\"";
  std::string seri = "\"" + snf::usb232x4pn + "\"";
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
    std::cout << "\033[31m Found too few or many US-RS232 devices. They must be exactly FOUR. Exit...\033[0m" << std::endl;
    exit(1);
  }

  for( int i=0; i<4; i++ ) {
    bool alive = false;
    Bronko* aBronkhorst = new Bronko( foundDev[i] );
    try {
      alive = aBronkhorst->serialConnect();      
    } catch( std::string error ) {
      std::cout << "\033[31mError connecting to device " << foundDev[i] << ": "
		<< error << "\033[0m"  << std::endl;
      aBronkhorst->serialDisconnect();
      exit(1);
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
	throw( "Bronkhorst with s/n "+sn+" not in list" );
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
  return; 
}

// <><><><><><> I needed this just for debugging...
void Bronko::_dumpString( std::string s ) {
  for( int i=0; i<s.size(); i++ ) {
    char c = s.substr(i,1).c_str()[0];
    std::cout << std::hex << int(c) << std::dec;
    if( int(c)>31 && int(c)<127 ) std::cout << " " << c;
    std::cout << std::endl;
  }
}

// <><><><><><> Used for "hit <enter> to exit"...
int Bronko::_kbEnter( void ) {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

// <><><><><><> To convert ASCII string to char string...
std::string Bronko::_ascii2string( std::string s ) {
  std::string r;
  for( int i=0; i<s.size(); i+=2 ) {
    r.push_back( static_cast<char>( strtol( s.substr(i,2).c_str(), NULL, 16 ) ) );
  }
  return( r );
}

// <><><><><><> To convert ASCII string to 16 bit inreger...
int16_t Bronko::_ascii2int16( std::string s ) {
  return( static_cast<int16_t>( strtol( s.c_str(), NULL, 16 ) ) );
}

// <><><><><><> To convert ASCII string to double precision floating point...
float Bronko::_ascii2float( std::string s ) {
  union{ int32_t i; float d; } u;
  u.i = static_cast<int32_t>( strtol( s.c_str(), NULL, 16 ) );
  return( u.d );
}


// <><><><><><> Default Constructor
Bronko::Bronko( std::string device ) : _device(device), _fd(0) {
  _resetReadTimeout();
}

// <><><><><><> Scale readout timeout
const float _readTimeout0 = 0.12;
void Bronko::_scaleReadTimeout( float scale ) {
  _readTimeout = _readTimeout0 * scale;
  return;
}

// <><><><><><> Reset readout timeout
void Bronko::_resetReadTimeout( void ) {
  _readTimeout = _readTimeout0;
  return;
}

// <><><><><><> try to write some characters to serial
void Bronko::_serialWrite( std::string data ) {
  int status = write( _fd, data.c_str(), data.size() );
  if( status != data.size() ) {
    throw( std::string( strerror( errno ) ) );
    return;
  }
}

// <><><><><><> try to read something from seriar (slow but safer...)
std::string Bronko::_serialRead() {

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
std::string Bronko::_serialFastRead() {

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
bool Bronko::serialConnect( void ) {

  _fd = open( _device.c_str(), O_RDWR | O_NOCTTY );
  if( _fd < 0 ) {
    throw( std::string( strerror( errno ) ) );
    return false;
  }  

  tcgetattr( _fd, &_oldtio );
  _newtio = _oldtio;

  cfsetispeed( &_newtio, (speed_t)B38400 );
  cfsetospeed( &_newtio, (speed_t)B38400 );
  cfmakeraw(   &_newtio );

  tcsetattr( _fd, TCSANOW, &_newtio );  
  tcflush( _fd, TCIOFLUSH );
  
  // A little bit tricky: I send an simple command to the "potential"
  // Bronkhorst and I check if I get a meningful answer...

  std::string snum = ":0780047163716300\r\n";
  int status = write( _fd, snum.c_str(), snum.size() );
  usleep( 200000 );
  fd_set read_fds, write_fds, except_fds;
  FD_ZERO( &read_fds );
  FD_ZERO( &write_fds );
  FD_ZERO( &except_fds );
  FD_SET( _fd, &read_fds );
  struct timeval timeout;
  timeout.tv_usec = 0;
  timeout.tv_sec  = 1;
  char buff[512];
  int rv = select( _fd+1, &read_fds, &write_fds, &except_fds, &timeout );
  if( rv > 0 ) {
    _mySN = _getSerialNumber();
    return true;
  }
  else {
    return false;
  }
}

// <><><><><><> Restore default Serial Settings and close it
void Bronko::serialDisconnect( void ) {

  tcsetattr( _fd, TCSANOW, &_oldtio ); /* restore old port settings */
  tcflush( _fd, TCIOFLUSH );
  if( close( _fd ) < 0 ) {
    throw ( std::string( strerror( errno ) ) );
  } 
  return;
}

// <><><><><><> Commands...
// <><><><><><>
std::string Bronko::_getSerialNumber( void ) {
  try {
    std::string command = ":0780047163716300\r\n";
    _serialWrite( command );
    usleep( 200000 ); // whait a while to let the device think a little...
    std::string answer = _serialRead();
    int to = answer.rfind( "00" ), from = answer.substr( 0, to ).rfind( "00" );
    answer = answer.substr( from+2, to-from-2 );
    return( _ascii2string( answer ) );
  } catch( std::string error) {
    std::cout << "\033[31mError during comunication: " << error << "\033[0m"  << std::endl;
    return( "" );
  }
}

// <><><><><><>
std::string Bronko::getModelName( void ) {
  try {
    std::string command = ":0703047162716200\r\n";
    _serialWrite( command );
    usleep( 500000 ); // whait a while to let the device think a little...
    std::string answer = _serialRead();
    int to = answer.rfind( "00" ), from = answer.substr( 0, to ).rfind( "00" );
    answer = answer.substr( from+2, to-from-2 );
    return( _ascii2string( answer ) );
  } catch( std::string error) {
    std::cout << "\033[31mError during comunication: " << error << "\033[0m"  << std::endl;
    return( "" );
  }
}

// <><><><><><>
int16_t Bronko::getMeasureInt16( void ) {
  try {
    std::string command = ":06800401210120\r\n";
    _serialWrite( command );
    usleep( 500000 ); // whait a while to let the device think a little...
    std::string answer = _serialRead();
    if( answer.substr( 0, 11 ) != ":0680020121" ) throw( "Device answer syntax not correct: " + answer );
    int to = answer.rfind( "\r" );
    answer = answer.substr( to-4, 4 );
    return( _ascii2int16( answer ) );
  } catch( std::string error) {
    std::cout << "\033[31mError during comunication: " << error << "\033[0m"  << std::endl;
    return( -9999 );
  }
}

// <><><><><><>
float Bronko::getMeasureFloat( void ) {
  try {
    std::string command = ":06800421402140\r\n";
    _serialWrite( command );
    usleep( 500000 ); // whait a while to let the device think a little...
    std::string answer = _serialRead();
    if( answer.substr( 0, 11 ) != ":0880022140" ) throw( "Device answer syntax not correct: " + answer );
    int to = answer.rfind( "\r" );
    answer = answer.substr( to-8, 8 );
    return( _ascii2float( answer ) );
  } catch( std::string error) {
    std::cout << "\033[31mError during comunication: " << error << "\033[0m"  << std::endl;
    return( -9999 );
  }
}

// <><><><><><>
float Bronko::getTemperature( void ) {
  try {
    std::string command = ":06800421472147\r\n";
    _serialWrite( command );
    usleep( 500000 ); // whait a while to let the device think a little...
    std::string answer = _serialRead();
    if( answer.substr( 0, 11 ) != ":0880022147" ) throw( "Device answer syntax not correct: " + answer );
    int to = answer.rfind( "\r" );
    answer = answer.substr( to-8, 8 );
    return( _ascii2float( answer ) );
  } catch( std::string error) {
    std::cout << "\033[31mError during comunication: " << error << "\033[0m"  << std::endl;
    return( -9999 );
  }
}

// <><><><><><> Careful: should run fast, but it's unsafe!
void Bronko::showMeasurementContinuously( void ) {
  std::cout << "\033[37mCountinuous Loop on device \033[0m(Hit \033[1;36m<Enter>\033[0m to exit)..." << std::endl;
  while( true ) {
    std::string command = ":06800421402140\r\n";
    _serialWrite( command );
    usleep( 50000 );
    std::string answer = _serialFastRead();
    if( answer.substr( 0, 11 ) == ":0880022140" ) {
      int to = answer.rfind( "\r" );
      answer = answer.substr( to-8, 8 );
      float flux = _ascii2float( answer );
      std::cout << "\033[0mFlux (l/min): \033[1;33m" << flux << "\033[0m" << std::endl;
      std::cout << "\033[2K\033[1A";
    }
    if( _kbEnter() != 0 ) return;
  }
}
