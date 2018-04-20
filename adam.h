#ifndef ADAM_H
#define ADAM_H

// +-------------------------------------+
// | ADAM-4019+                          |
// | serial wrapper                      |
// | Benigno Gobbo INFN Trieste          |
// | 20170620 V1.2                       |
// | 20180419 V2.0 Loop on devices moved |
// |               into init             |
// +-------------------------------------+

#include <string>
#include <termios.h>
#include <exception>

class Adam {

 public:

  Adam( std::string device );
  bool serialConnect( void );
  void serialDisconnect( void );

  // Those are functions allowing some chats with the adam...
  inline std::string getAddress( void ) { return( _myAddr ); }
  double getMeasurement ( int channel );
  
 private:

  std::string    _device;
  std::string    _myAddr;
  int            _fd;
  double         _readTimeout;
  char           _buff[256];
  struct termios _oldtio;
  struct termios _newtio;
  
  void _serialWrite( std::string data ); 
  std::string _serialRead( void );
  std::string _serialFastRead( void );
  void _scaleReadTimeout( float scale );
  void _resetReadTimeout( void );
  std::string _checkChannel( int channel );
  
  std::string _getAddress( void );
  std::string _getSerialNumber( void );
  
};
  
#endif // Adam_H
