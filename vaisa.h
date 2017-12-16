#ifndef VAISA_H
#define VAISA_H

// +-------------------------------------+
// | Vaisala DMT143 Dewpoint Transmitter |
// | serial wrapper                      |
// | Benigno Gobbo INFN Trieste          |
// | 20170620 V1.1                       |
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
  inline int         getAddr( void ) { return _myAddr; }
  float getTdf( int ndec = 4 );
  float getTdfAtm( int ndec = 4 );
  int   getH2O( void );
  
 private:

  std::string    _device;
  int            _myAddr;
  std::string    _mySN;
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
  void _dumpString( std::string s );
};


class VaisaInit {

 public:
  static VaisaInit* initialize( void );
  inline std::vector<Vaisa*> getVaisas( void ) { return _vaisas; }

 protected:
  VaisaInit();

 private:
  static VaisaInit* _init;
  static std::vector<Vaisa*> _vaisas;
  
};

#endif // Vaisa_H
