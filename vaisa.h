#ifndef VAISA_H
#define VAISA_H

// +-------------------------------------+
// | Vaisala DMT1xx Dewpoint Transmitter |
// | serial wrapper                      |
// | Benigno Gobbo INFN Trieste          |
// | 20170620 V1.1                       |
// | 20180419 V2.0 Loop on devices moved |
// |               into init             |
// | 20181018 V2.1 Reset function        |
// | 20181116 V2.2 Added DMT152+fixes    | 
// +-------------------------------------+

#include <string>
#include <vector>
#include <termios.h>
#include <exception>

class Vaisa {

 public:

  Vaisa( std::string device );
  ~Vaisa() {};

  bool serialConnect( void );
  void serialDisconnect( void );
  std::string getDataString( void );
  template <typename T> std::string sendCommand( std::string command, T argValue );
  std::string sendCommand( std::string command );
  inline std::string getDevice( void ) { return _device; }
  inline std::string getSerialNumber( void ) { return _mySN; }
  inline std::string getProductType( void ) { return _myPT; }
  inline int         getAddr( void ) { return _myAddr; }
  float getTdf( int ndec = 4 );
  float getTdfAtm( int ndec = 4 );
  int   getH2O( void );
  bool  factoryReset( void );
  
 private:

  std::string    _device;
  int            _myAddr;
  std::string    _mySN;
  std::string    _myPT;
  int            _fd;
  double         _readTimeout;
  char           _buff[256];
  struct termios _oldtio;
  struct termios _newtio;

  void _serialWrite( std::string data ); 
  std::string _serialRead( void );
  std::string _getValue( std::string value, std::string precision );
  void _scaleReadTimeout( float scale );
  void _resetReadTimeout( void );
  int _getMyAddr( void );
  std::string _getMySN( void );
  std::string _getMyPT( void );
  void _dumpString( std::string s );
  bool _isFloat( std::string s );
  bool _isInt( std::string s );
};

#endif // Vaisa_H
