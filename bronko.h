#ifndef BRONKO_H
#define BRONKO_H

// +-------------------------------------+
// | Bronkhorst flow meter               |
// | serial wrapper                      |
// | Benigno Gobbo INFN Trieste          |
// | 20170620 V1.1                       |
// | 20180419 V2.0 Loop on devices moved |
// |               into init             |
// +-------------------------------------+

#include <string>
#include <vector>
#include <termios.h>
#include <exception>

class Bronko {

 public:

  Bronko( std::string device );
  ~Bronko() {};

  bool serialConnect( void );
  void serialDisconnect( void );

  // Those are functions allowing some chats with the bronko...
  inline std::string getSerialNumber( void ) { return( _mySN ); }
  std::string getModelName( void );
  int16_t getMeasureInt16( void );
  float getMeasureFloat( void );
  float getTemperature( void );
  void showMeasurementContinuously( void );
  
 private:

  std::string    _mySN;
  std::string    _device;
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

  void _dumpString( std::string s );
  int  _kbEnter( void );

  std::string _getSerialNumber( void );
  std::string _ascii2string( std::string s );
  int16_t     _ascii2int16( std::string s );
  float       _ascii2float( std::string s );
  
};

#endif // Bronko_H
