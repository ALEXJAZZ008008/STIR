//
// $Id$
//
/*!
  \file
  \ingroup motion

  \brief Declaration of class RigidObject3DMotionFromPolaris

  \author  Sanida Mustafovic and Kris Thielemans
  $Date$
  $Revision$ 
*/

/*
    Copyright (C) 2003- $Date$, Hammersmith Imanet
    See STIR/LICENSE.txt for details
*/


#include "local/stir/motion/RigidObject3DMotion.h"
#include "local/stir/motion/Polaris_MT_File.h"
#include "stir/RegisteredParsingObject.h"

START_NAMESPACE_STIR

class RigidObject3DMotionFromPolaris: 
  public 
  RegisteredParsingObject< RigidObject3DMotionFromPolaris,
			   RigidObject3DMotion,
			   RigidObject3DMotion>
                                                             
{
public:
  //! Name which will be used when parsing a MotionTracking object 
  static const char * const registered_name; 

  // only need this to enable LmToProjDataWithMC(const char * const par_filename) function
  RigidObject3DMotionFromPolaris();

  RigidObject3DMotionFromPolaris(const string mt_filename,shared_ptr<Polaris_MT_File> mt_file_ptr);

  //! Find average motion from the Polaris file 
  virtual RigidObject3DTransformation 
    compute_average_motion(const double start_time, const double end_time) const;

  //! Given the time obtain motion info, i.e. RigidObject3DTransformation
  virtual void get_motion(RigidObject3DTransformation& ro3dtrans, const double time) const;

  //! Synchronise motion tracking file and listmode file
  virtual Succeeded synchronise(CListModeData& listmode_data);

  virtual const RigidObject3DTransformation& 
    get_transformation_to_scanner_coords() const;
  virtual const RigidObject3DTransformation& 
    get_transformation_from_scanner_coords() const;
  
//private: 
  //! Find and store gating values in a vector from lm_file  
  void find_and_store_gate_tag_values_from_lm(vector<float>& lm_time, 
					      vector<unsigned>& lm_random_number,
					      CListModeData& listmode_data);

  //! Find and store random numbers from mt_file
  void find_and_store_random_numbers_from_mt_file(vector<unsigned>& mt_random_numbers);

  void find_offset(CListModeData& listmode_data);



  shared_ptr<Polaris_MT_File> mt_file_ptr;
  string mt_filename;  
  //string lm_filename;
 
#if 1
  virtual void set_defaults();
  virtual void initialise_keymap();
  virtual bool post_processing();
#endif

 private:
  string transformation_from_scanner_coordinates_filename;
  RigidObject3DTransformation move_to_scanner_coords;
  RigidObject3DTransformation move_from_scanner_coords;

};


END_NAMESPACE_STIR
