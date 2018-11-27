// +-------------------------------------+
// | Loop and write on display           |
// | Benigno Gobbo INFN Trieste          |
// | 20171211                            |
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

  std::cout << "\033c";
  
  while( true ) {
    time_t now = time( 0 ); // seconds since epoch
    struct tm ts;
    ts = *localtime( &now );
    char tbuff[80];
    strftime( tbuff, sizeof(tbuff), "%Y-%m-%d %X", &ts );
    std::cout << "\033[H\033[2K\033[1;34mDate: " << tbuff << "\033[0m" << std::endl;

    // The Vaisalas
    std::cout << "The Vaisala " << vaisas[0]->getProductType() << " Dewpoint Transmitters:" << std::endl;
    float tdf = 0; int vStat = 0;  
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
	} catch( std::string error ) {
	  std::cout << "\033[31mError reading data string device " << vaisas[i]->getDevice() << ": "
		    << error << "\033[0m"  << std::endl;
	  vaisas[i]->serialDisconnect();
	  return(1);
	}      
	std::cout << "\033[2K\033[36mLine " << i+1 << ": \033[37m"
		  << std::setw(6) << std::setprecision(2) << std::fixed << std::showpoint << tdf << " 'C\033[0m" << std::endl;
      }
      else {
	std::cout << "\033[2K\033[36mLine " << i+1 << ": ------ \033[0m" << std::endl;
      }
    }
    
    // The Bronkhorsts
    std::cout << "The Bronkhorst F-101E-AGD-33-V Mass Flow Meters:" << std::endl;
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
	std::cout << "\033[2K\033[36mLine " << i+1 << ": \033[37m"
		  << std::setw(7) << std::setprecision(4) << std::fixed << std::showpoint << flux << " l/min\033[36m, "
		  << std::setw(5) << std::setprecision(2) << std::fixed << std::showpoint << temp << " 'C\033[0m"
		  << std::endl;
      }
      else {
	std::cout << "\033[2K\033[36mLine " << i+1 << ": ----------------------- \033[0m" << std::endl;
      }
    }
  
    // The Adam
    std::cout << "The ADAM-4019+ Analog Input Module (Pressure Transmitters):" << std::endl;
    for( int i=0; i<(snf::busegem.size()-2); i++ ) {
      if( snf::busegem.test(i) && snf::bgotgem.test(i) ) {
	try{
	  double meas = adam->getMeasurement( i );
	  double pres = meas * 25.0 + 700.0;
	  std::cout << "\033[2K\033[36mLine " << i+1 << ": \033[37m"
		    << std::setw(6) << std::setprecision(1) << std::fixed << std::showpoint << pres << " mbar \033[36m("
		    << std::setw(5) << std::setprecision(2) << std::fixed << std::showpoint << meas << " mA)\033[0m"
		    << std::endl;
	  
	} catch( std::string error ) {
	  std::cout << "\033[31mError reading data from channel " << i << ": "
		    << error << "\033[0m"  << std::endl;
	}
      }
      else {
	std::cout << "\033[2K\033[36mDevice " << i << ": ---------------------- \033[0m" << std::endl;
      }
    }

    for( int j=0; j<sleepSecs; j++ ) {
      std::cout << "\033[2K\033[32mNow sleeping " << sleepSecs-j
                << " second(s) \033[34m(<Enter> to exit)\033[0m" << std::endl;
      int hit = kbhit();
      if( hit != 0 ) {
	std::cout << std::endl << "\033[4mYou get bored, fine. Cheerio, folks. \033[0m"
		  << std::endl << std::endl;
        exit(0);
      }
      std::cout << "\033[2A" << std::endl;
    }
    std::cout << "\033[2K" << std::endl;
  }

  return(0);
}
