/*
 *   This source code is part of the Eutelescope package of Marlin.
 *   You are free to use this source files for your own development as
 *   long as it stays in a public research context. You are not
 *   allowed to use it for commercial purpose. You must put this
 *   header with author names in all development based on this file.
 *
 */

// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelExplorer30Detector.h"

// system includes <>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;
using namespace eutelescope;

EUTelExplorer30Detector::EUTelExplorer30Detector() : EUTelPixelDetector()  {

  _xMin = 0;
  _xMax = 59;

  _yMin = 0;
  _yMax = 59;

  _signalPolarity = 1;

  _name = "Explorer30";

  _xPitch = 0.03;
  _yPitch = 0.03;

  // now the matrix boundaries with markers			//subchannels can be used to specify the substructure
  EUTelROI ch0wm(   0,  0, 59, 59 );				//of a sensor eg. 9 different architectures

  _subChannelsWithMarkers.push_back( ch0wm );

  // now the matrix boundaries without markers
  EUTelROI ch0wom(   0,  0, 59, 59 );

  _subChannelsWithoutMarkers.push_back( ch0wom );

}

bool EUTelExplorer30Detector::hasSubChannels() const {
  if (  _subChannelsWithoutMarkers.size() != 0 ) return true;
  else return false;
}

std::vector< EUTelROI > EUTelExplorer30Detector::getSubChannels( bool withMarker ) const {

  if ( withMarker ) return _subChannelsWithMarkers;
  else  return _subChannelsWithoutMarkers;

}

EUTelROI EUTelExplorer30Detector::getSubChannelBoundary( size_t iChan, bool withMarker ) const {
  if ( withMarker ) return _subChannelsWithMarkers.at( iChan );
  else return _subChannelsWithoutMarkers.at( iChan );

}


void EUTelExplorer30Detector::setMode( string mode ) {

  _mode = mode;

}

void EUTelExplorer30Detector::print( ostream& os ) const {

  size_t w = 35;

  string pol = "negative";
  if ( _signalPolarity > 0 ) pol = "positive";

  os << resetiosflags(ios::right)
     << setiosflags(ios::left)
     << setfill('.') << setw( w ) << setiosflags(ios::left) << "Detector name " << resetiosflags(ios::left) << " " << _name << endl
     << setw( w ) << setiosflags(ios::left) << "Mode " << resetiosflags(ios::left) << " " << _mode << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel along x " << resetiosflags(ios::left) << " from " << _xMin << " to " << _xMax << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel along y " << resetiosflags(ios::left) << " from " << _yMin << " to " << _yMax << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel pitch along x " << resetiosflags(ios::left) << " " << _xPitch << "  mm  "  << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel pitch along y " << resetiosflags(ios::left) << " " << _yPitch << "  mm  "  << endl
     << setw( w ) << setiosflags(ios::left) << "Signal polarity " << resetiosflags(ios::left) << " " << pol <<  setfill(' ') << endl;

  if ( hasMarker() ) {

    os << resetiosflags( ios::right ) << setiosflags( ios::left );
    os << "Detector has the following columns (" << _markerPos.size() << ") used as markers: "<< endl;

    vector< size_t >::const_iterator iter = _markerPos.begin();
    while ( iter != _markerPos.end() ) {

      os << "x = " << setw( 15 ) << setiosflags(ios::right) << (*iter) << resetiosflags(ios::right) << endl;

      ++iter;
    }

  }
}
