#ifndef INIT_ALL_H
#define INIT_ALL_H

// +--------------------------------------------+
// | Initialization                             |
// |                                            |
// | Benigno Gobbo INFN Trieste                 |
// | 20170619 V1.0                              |
// | 20170727 V1.1 Use JSON                     |
// | 20170915 V1.1.1 Fixes                      |
// | 20180418 V2.0 Loop on devices moved here   |
// | 20181116 V2.1 No dev scan if not selected  |
// +--------------------------------------------+

#include <vector>
#include "devdata.h"
#include "vaisa.h"
#include "adam.h"
#include "bronko.h"

class Init {
  
 public:
  static Init* initialize( void );
  inline Adam*                getAdam( void )    { return _adam; }
  inline std::vector<Vaisa*>  getVaisas( void )  { return _vaisas; }
  inline std::vector<Bronko*> getBronkos( void ) { return _bronkos; }
  void reconnectDevices( void );
  
 protected:
  Init();

 private:
  static Init* _init;
  static std::vector<Bronko*> _bronkos;
  static std::vector<Vaisa*>  _vaisas;
  static Adam*                _adam;
  // functions
  void   readJSON( void );
  void   searchDevices( void );
  void   cleanDevices( void );  
};
  
#endif // INIT_ALL_H
