//
// $Id$
//
#ifndef __stir_FBP2D_FBP2DReconstruction_H__
#define __stir_FBP2D_FBP2DReconstruction_H__
/*!
  \file 
  \ingroup recon_buildblock
 
  \brief declares the FBP2DReconstruction class

  \author Kris Thielemans
  \author PARAPET project

  $Date$
  $Revision$
*/
/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000- $Date$, Hammersmith Imanet
    See STIR/LICENSE.txt for details
*/

#include "stir/recon_buildblock/Reconstruction.h"
#include <string>

#ifndef STIR_NO_NAMESPACES
using std::string;
#endif

START_NAMESPACE_STIR

template <int num_dimensions, typename elemT> class DiscretisedDensity;
template <typename T> class shared_ptr;
class Succeeded;
class ProjData;

//! Reconstruction class for 2D Filtered Back Projection
class FBP2DReconstruction : public Reconstruction
{

public:
  //! Default constructor (calls set_defaults())
  FBP2DReconstruction (); 
  /*!
    \brief Constructor, initialises everything from parameter file, or (when
    parameter_filename == "") by calling ask_parameters().
  */
  explicit 
    FBP2DReconstruction(const string& parameter_filename);

  FBP2DReconstruction(const shared_ptr<ProjData>&, 
		      const double alpha_ramp=1.,
		      const double fc_ramp=.5,
		      const int pad_in_s=2,
		      const int num_segments_to_combine=-1
		      );
  
  Succeeded reconstruct(shared_ptr<DiscretisedDensity<3,float> > const &);
  
   //! Reconstruction that gets target_image info from the parameters
   /*! sadly have to repeat that here, as Reconstruct::reconstruct() gets hidden by the 
       1-argument version.
       */
  virtual Succeeded reconstruct();

  virtual string method_info() const;

  virtual void ask_parameters();

 protected: // make parameters protected such that doc shows always up in doxygen
  // parameters used for parsing

  //! Ramp filter: Alpha value
  double alpha_ramp;
  //! Ramp filter: Cut off frequency
  double fc_ramp;  
  //! amount of padding for the filter (has to be 0,1 or 2)
  int pad_in_s;
  //! number of segments to combine (with SSRB) before starting 2D reconstruction
  /*! if -1, a value is chosen depending on the axial compression.
      If there is no axial compression, num_segments_to_combine is
      effectively set to 3, otherwise it is set to 1.
      \see SSRB
  */
  int num_segments_to_combine;

 private:
  virtual void set_defaults();
  virtual void initialise_keymap();
  virtual bool post_processing(); 
  bool post_processing_only_FBP2D_parameters();

};




END_NAMESPACE_STIR

    
#endif

