// +-------------------------------------+
// | Loop on sensors and dump on file    |
// | Benigno Gobbo INFN Trieste          |
// | 20171206                            |
// +-------------------------------------+

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include "init.h"

const int sleepSecs = 3;

// <+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+>
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

// <+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+>
int main( void ) {

  // The data
  float dewp[4], flux[4], pres[6];
  std::string outFileName = "sensors_data.txt";
  
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

  std::cout << std::endl
	    << "-----------------------------------------------------------------" << std::endl
	    << "All available connections established. Starting the measurements." << std::endl
	    << "Hit <Enter> key to exit)..." << std::endl
    	    << "-----------------------------------------------------------------" << std::endl
	    << std::endl;
  
  std::fstream outfile( outFileName, std::ios_base::out | std::ios_base::trunc );
  if( outfile.fail() ) {
    std::cout << "\033[31mError in opening '" << outFileName << "'.\033[0m"  << std::endl;
    return(1);
  }

  // The infinite loop...
  
  while( true ) {

    // The Vaisalas
    for( int i=0; i<vaisas.size(); i++ ) {
      dewp[i] = 0;
      if( snf::busevai.test(i) && snf::bgotvai.test(i) ) {
	try {
	  dewp[i] = vaisas[i]->getTdf( 3 );
	} catch( std::string error ) {
	  std::cout << "\033[31mError reading data string device " << vaisas[i]->getDevice() << ": "
		    << error << "\033[0m"  << std::endl;
	  outfile.close();
	  return(1);
	}      
      }
    }

    // The Bronkhorsts
    for( int i=0; i<bronkos.size(); i++ ) {
      flux[i] = 0;
      if( snf::busebro.test(i) && snf::bgotbro.test(i) ) {
	try {
	  flux[i] = bronkos[i]->getMeasureFloat();
	} catch( std::string error ) {
	  std::cout << "\033[31mError reading data string device " << bronkos[i]->getSerialNumber() << ": "
		    << error << "\033[0m"  << std::endl;
	  outfile.close();
	  return(1);
	}      
      }
    }
  
    // The Adam
    for( int i=0; i<snf::busegem.size(); i++ ) {
      pres[i] = 0;
      if( snf::busegem.test(i) && snf::bgotgem.test(i) ) {
	try{
	  double meas = adam->getMeasurement( i );
	  pres[i] = meas * 25.0 + 700.0;
	} catch( std::string error ) {
	  std::cout << "\033[31mError reading data from channel " << i << ": "
		    << error << "\033[0m"  << std::endl;
	}
      }
    }

    // Save data
    for( int i=0; i<4; i++ ) {
      outfile << dewp[i] << " " << flux[i] << " " << pres[i] << " ";
    }
    outfile << pres[4] << " " << pres[5] << std::endl;

    int hit = kbhit();
    if( hit != 0 ) {
      outfile.close();
      return(0);
    }

  }

  // Should never arrive here, but anyhow...
  outfile.close();
  
  return(0);
}
