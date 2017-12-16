#ifndef INIT_ALL_H
#define INIT_ALL_H

// +-------------------------------------+
// | Initialization                      |
// |                                     |
// | Benigno Gobbo INFN Trieste          |
// | 20170619 V1.0                       |
// +-------------------------------------+

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

 protected:
  Init();

 private:
  static Init* _init;
  static std::vector<Bronko*> _bronkos;
  static std::vector<Vaisa*>  _vaisas;
  static Adam*                _adam;
  
};
  
#endif // INIT_ALL_H
