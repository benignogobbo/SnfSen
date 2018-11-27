// +-------------------------------------+
// | Just a test program                 |
// | Benigno Gobbo INFN Trieste          |
// | 20171120                            |
// +-------------------------------------+

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include "init.h"

const int sleepSecs = 3;
int kbhit() {

    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

int main( void ) {

  // Initialize
  std::vector<Vaisa*>  vaisas;
  std::vector<Bronko*> bronkos;
  Adam*                adam;
  
  try {
    Init* init = Init::initialize();
    vaisas  = init->getVaisas();
    bronkos = init->getBronkos();
    adam    = init->getAdam();
  } catch( std::string error ) {
    std::cout << "\033[31mError during initialisation: " << error << "\033[0m"  << std::endl;
    return(1);
  }

  std::cout << std::endl << "\033[4mLet's now contact all active devices for some measurements\033[0m"
	    << std::endl << std::endl;
  
  // The Vaisalas
  std::cout << "Vaisalas test:" << std::endl;
  float tdf = 0; int vStat = 0; std::string vErrs = "";  
  for( int i=0; i<vaisas.size(); i++ ) {
    if( snf::busevai.test(i) && snf::bgotvai.test(i) ) {
      try {
	tdf = 0;
	if( vaisas[i]->isaDMT152() ) {
	  vStat = vaisas[i]->getStatus();
	  if( vStat%10 == 0 ) tdf = vaisas[i]->getTdf( 3 );
	}
	else if( vaisas[i]->isaDMT143() ) {
	  tdf = vaisas[i]->getTdf( 3 );
	}
	else {
	  tdf = 0;
	}
	vErrs = vaisas[i]->getErrors();
      } catch( std::string error ) {
	std::cout << "\033[31mError reading data string device " << vaisas[i]->getDevice() << ": "
		  << error << "\033[0m"  << std::endl;
	vaisas[i]->serialDisconnect();
	return(1);
      }      
      std::cout << "\033[36mDevice " << i << ": " << tdf << " 'C\033[0m" << std::endl;
    }
    else {
      std::cout << "\033[36mDevice " << i << ": --- \033[0m" << std::endl;
    }
  }

  // The Bronkhorsts
  std::cout << "Bronkhorsts test:" << std::endl;
  float flux = 0, temp = 0;
  for( int i=0; i<bronkos.size(); i++ ) {
    if( snf::busebro.test(i) && snf::bgotbro.test(i) ) {
      try {
	flux = bronkos[i]->getMeasureFloat();
        temp = bronkos[i]->getTemperature();
      } catch( std::string error ) {
	std::cout << "\033[31mError reading data string device " << bronkos[i]->getSerialNumber() << ": "
		  << error << "\033[0m"  << std::endl;
	bronkos[i]->serialDisconnect();
	return(1);
      }      
      std::cout << "\033[36mDevice " << i << ": " << flux << " l/min, "<< temp << " 'C\033[0m" << std::endl;
    }
    else {
      std::cout << "\033[36mDevice " << i << ": --- \033[0m" << std::endl;
    }
  }
  
  // The Adam
  std::cout << "Adam analog inputs test:" << std::endl;
  for( int i=0; i<snf::busegem.size(); i++ ) {
    if( snf::busegem.test(i) && snf::bgotgem.test(i) ) {
      try{
	double meas = adam->getMeasurement( i );
	double pres = meas * 25.0 + 700.0;
	std::cout << "\033[36mDevice " << i << ": " << meas
		  << " mA, i.e. " << pres << " mbar\033[0m" << std::endl;
      } catch( std::string error ) {
	std::cout << "\033[31mError reading data from channel " << i << ": "
		  << error << "\033[0m"  << std::endl;
      }
    }
    else {
      std::cout << "\033[36mDevice " << i << ": --- \033[0m" << std::endl;
    }
  }

  std::cout << std::endl << "\033[4mEnd of test procedures. Cheerio, folks. \033[0m"
	    << std::endl << std::endl;

  return(0);
}
