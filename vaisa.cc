// +-------------------------------------+
// | Vaisala DMT1xx Dewpoint Transmitter |
// | serial wrapper                      |
// | Benigno Gobbo INFN Trieste          |
// +-------------------------------------+

#include <iostream>
#include <string>
#include <sstream>
#include <typeinfo>
#include <cstring>
#include <cerrno>
#include <cmath>

#include <fcntl.h>
#include <unistd.h>

#include "vaisa.h"
#include "devdata.h"

#define MAX_ATTEMPTS (500)

// <><><><><><> This to check if a string is a real number...
bool Vaisa::_isFloat( std::string s ) {
    std::istringstream ss( s );
    float f;
    ss >> std::noskipws >> f;
    return ss.eof() && !ss.fail(); 
}

// <><><><><><> This to check if a string is an integer number...
bool Vaisa::_isInt( std::string s ) {
    std::istringstream ss( s );
    int i;
    ss >> std::noskipws >> i;
    return ss.eof() && !ss.fail(); 
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
    answer = answer.substr( answer.find("Address")+24,1 );
    if( _isInt( answer )) {
      addr = std::stoi( answer );
    }
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

// <><><><><><> Get device product type
std::string Vaisa::_getMyPT( void ) {

  std::string answer = "";
  try {
    std::string command = "vers\015"; // addr<cr>
    _serialWrite( command );
    _scaleReadTimeout( 3 );
    answer = _serialRead();
    answer = answer.substr( command.size(), 6 );
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
    int count = 0;
    bool ok = false;
    if( precision != "" ) {
      while( !ok && count++ < MAX_ATTEMPTS ) { // 20190315 Benigno: pedantic checks due to "connection to Vaisalas problem" 
	std::string command = "form " + precision + " " + value + " #000";  // form [string]
	status = sendCommand( command );
	if( status.length() > 2 ) {
	  if( status.substr(0,2) == "OK" ) {
	    command = "form";
	    status = sendCommand( command );
	    std::string check;
	    if( precision == "" ) {
	      check = value + " \\000 \r\n";
	    }
	    else {
	      check = precision + " " + value + " \\000 \r\n";
	    }
	    if( status  == check ) {
	      ok = true;
	    }
	  }
	}
      }
    }
    usleep( 100000 ); // wait a short while...
    std::string command = "send"; // send<cr>
    count = 0;
    do { // 20190315 Benigno. If no data try more and more times... 
      data = sendCommand( command );
      if( data.size() > 0 ) {
	size_t first = data.find_first_not_of(' ');
	size_t last = data.find_last_not_of(' ');
	if( first >= 0 && last >= 0 && first < data.length() && (last-first+1) <= data.length() ) { 
	  data = data.substr(first, (last-first+1));
	  if( _isFloat( data ) ) {
	    return(data);
	  }
	  else {
	    throw( std::string( "Error, what read is not a number." ) );
	    return(std::string(""));
	  } 
	}
      }
      usleep( 50000 );
    } while( count++ < MAX_ATTEMPTS );
  } catch( std::string error ) {
    throw( error );
    return(std::string(""));
  }
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
  _myPT = _getMyPT();
  
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
    val = std::stof( _getValue( "Tdf", prec ) );
  } catch( std::string error ) {
    throw( error );
    return(0);
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
    return(0);
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
    return(0);
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

  if( command == "reset" ) sleep( 4 ); // reset requires 3-4s
  else if( command == "frestore" ) sleep( 1 );
  else usleep( 100000 );

  std::string s = _serialRead();
  if( s.length() > ss.str().length() ) {
    s = s.substr( ss.str().size() );
  }
  else {
    s = "";
  }

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

// <><><><><><> reset + frestore + addr <n>
bool Vaisa::factoryReset( void ) {
  std:: string s = "";
  int myAddr = _myAddr; 
  // Try a reset...
  try {
    s = sendCommand( "reset" ); // will take about 5s
  } catch( std::string error ) {
    throw( error );
    return( false );
  }
  if(  _myPT == "DMT143" && s.substr( 0,6 ) != "DMT143" ) {
    throw( std::string( "Error: Reset command failed" ) );
    return( false );
  }

  if(  _myPT == "DMT152" && s.substr( 0,6 ) != "DMT152" ) {
    throw( std::string( "Error: Reset command failed" ) );
    return( false );
  }
  // Try a factory restore...
  try {
    s = sendCommand( "frestore" );
  } catch( std::string error ) {
    throw( error );
    return( false );
  }
  if( s.substr( 0, 7 ) != "Factory" ) {
    throw( std::string( "Error: Factory Restore command failed" ) );
    return( false );
  }
  // Try set its address...
  try {
    s = sendCommand( "addr", myAddr );
  } catch( std::string error ) {
    throw( error );
    return( false );
  }  
  return( true );
}

// <><><><><><> reset + frestore + addr <n>
int Vaisa::getStatus( void ) {
  int val = 0;
  if( _myPT == "DMT152" ) {
    try {
      int count = 0;
      while( count++ < MAX_ATTEMPTS ) {
	std::string status = sendCommand( "STAT" );
	if( status.length() > 0 ) {
	  size_t first = status.find_first_not_of(' ');
	  if( first >= 0 && first+4 < status.length() ) { 
	    status = status.substr( first, first+4 );
	    if( _isInt( status ) ) {
	      val = std::stoi( status );
	      return( val );
	    }
	  }
	}
      }
    } catch( std::string error ) {
      throw( error );
      return(0);
    }
  }
  else {
    throw( std::string( "ERROR: this function works only on DMT152 devices." ) );
    return( 0 );
  }
}
// <><><><><><> reset + frestore + addr <n>
std::string Vaisa::getErrors( void ) {
  std::string s = "";
  try {
    s = sendCommand( "ERRS" );
    s = s.substr( 0, s.length()-2 );
  } catch( std::string error ) {
    throw( error );
    return( std::string("") );
  }
  return(s);
}
