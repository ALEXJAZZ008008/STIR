//
// $Id$
//

/*!
  \file 
  \ingroup densitydata
 
  \brief Implementations of non-inline functions of class DiscretisedDensity

  \author Kris Thielemans 
  \author PARAPET project

  $Date$
  $Revision$

*/
/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000- $Date$, IRSL
    See STIR/LICENSE.txt for details
*/
#include "stir/DiscretisedDensity.h"
#include "stir/IO/interfile.h"
#include "stir/IO/ecat6_utils.h"
#include "stir/IO/stir_ecat6.h"
#include "stir/IO/stir_ecat7.h"
#include "stir/VoxelsOnCartesianGrid.h"
#include "stir/is_null_ptr.h"
#include <typeinfo>
#include <fstream>

#ifndef STIR_NO_NAMESPACES
using std::fstream;
#endif

START_NAMESPACE_STIR

// sadly, gcc 2.95.* does not support local namespaces as used below
// This is slightly funny as it does work in ProjData.cxx. 
// Maybe because here it's in a template?
#   if __GNUC__ == 2 
USING_NAMESPACE_ECAT
#ifdef HAVE_LLN_MATRIX
USING_NAMESPACE_ECAT7
#endif
USING_NAMESPACE_ECAT6
#endif

/*! 
   This function will attempt to determine the type of image in the file,
   construct an object of the appropriate type, and return a pointer to 
   the object.

   The return value is a shared_ptr, to make sure that the 
   object will be deleted.

   If more than 1 image is in the file, only the first image is read.

   Currently only Interfile, ECAT6 and ECAT7 file formats are supported. 
   The image corresponding to frame 1 (and gate=1, data=1, bed=0 for CTI
   formats) in the file will be read. Note that ECAT7 support depends on 
   HAVE_LLN_MATRIX being defined.

   Developer's note: ideally the return value would be an auto_ptr, but 
   it seems to be difficult (impossible?) to assign auto_ptrs to shared_ptrs 
   on older compilers (including VC 6.0).
*/
template<int num_dimensions, typename elemT>
DiscretisedDensity<num_dimensions,elemT> *
DiscretisedDensity<num_dimensions,elemT>::
 read_from_file(const string& filename)
{
  if (num_dimensions != 3 || typeid(elemT) != typeid(float))
    error("DiscretisedDensity::read_from_file currently only supports 3d float images\n");

  const int max_length=300;
  char signature[max_length];

  // read signature
  {
    fstream input(filename.c_str(), ios::in | ios::binary);
    if (!input)
      error("DiscretisedDensity::read_from_file: error opening file %s\n", filename.c_str());
    
    input.read(signature, max_length);
    signature[max_length-1]='\0';
  }
  // Interfile
  if (is_interfile_signature(signature))
  {
#ifndef NDEBUG
    warning("DiscretisedDensity::read_from_file trying to read %s as Interfile\n", 
	    filename.c_str());
#endif
    DiscretisedDensity<num_dimensions,elemT> * density_ptr =
      read_interfile_image(filename);
    if (!is_null_ptr(density_ptr))
      return density_ptr;
  }


    
#ifdef HAVE_LLN_MATRIX
  if (strncmp(signature, "MATRIX", 6) == 0)
  {
#ifndef NDEBUG
    warning("DiscretisedDensity::read_from_file trying to read %s as ECAT7\n", filename.c_str());
#endif
    USING_NAMESPACE_ECAT
    USING_NAMESPACE_ECAT7

    if (is_ECAT7_image_file(filename))
    {
      warning("\nReading frame 1, gate 1, data 0, bed 0 from file %s\n",
	      filename.c_str());
      string interfile_header_name;
      if (write_basic_interfile_header_for_ECAT7(interfile_header_name, filename, 1,1,0,0) ==
        Succeeded::no)
        return 0;
#ifndef NDEBUG
      warning("DiscretisedDensity::read_from_file wrote interfile header %s\nNow reading as interfile\n", 
        interfile_header_name.c_str());
#endif
      return 
        read_interfile_image(interfile_header_name);
    }
    else
    {
      if (is_ECAT7_file(filename))
	warning("DiscretisedDensity::read_from_file ECAT7 file %s is of unsupported file type\n", filename.c_str());
    }

  }
#endif // HAVE_LLN_MATRIX

  {
    // Try ECAT6
    // ECAT6  does not have a signature
#ifndef NDEBUG
    warning("DiscretisedDensity::read_from_file trying to read %s as ECAT6\n", filename.c_str());
#endif
    USING_NAMESPACE_ECAT;
    USING_NAMESPACE_ECAT6;
      if (is_ECAT6_image_file(filename))
      {
        ECAT6_Main_header mhead;
        FILE * cti_fptr=fopen(filename.c_str(), "rb"); 
        if(cti_read_ECAT6_Main_header(cti_fptr, &mhead)!=EXIT_SUCCESS) 
          error ("error reading main header in ECAT 6 file %s\n", filename.c_str());
        
	warning("\nReading frame 1, gate 1, data 0, bed 0 from file %s\n",
		filename.c_str());
        return
          ECAT6_to_VoxelsOnCartesianGrid(/*frame_num, gate_num, data_num, bed_num*/1,1,0,0,
          cti_fptr, mhead);
      }
  }


  error("DiscretisedDensity::read_from_file: %s seems to be in an unsupported file format\n",
	filename.c_str());
  return 0;

}


/******************************
 instantiations
 *****************************/
#ifdef _MSC_VER
// disable warning on pure virtuals which are not defined
#pragma warning(disable: 4661)
#endif

template class DiscretisedDensity<3,float>;

END_NAMESPACE_STIR
