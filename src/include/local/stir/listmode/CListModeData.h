//
// $Id$
//
/*!

  \file
  \ingroup buildblock  
  \brief Declaration of class CListModeData
    
  \author Kris Thielemans
      
  $Date$
  $Revision$
*/
/*
    Copyright (C) 2003- $Date$, IRSL
    See STIR/LICENSE.txt for details
*/

#ifndef __stir_listmode_CListModeData_H__
#define __stir_listmode_CListModeData_H__

#include "stir/Scanner.h"
#include "stir/shared_ptr.h"
#include <string>

#ifndef STIR_NO_NAMESPACES
using std::string;
#endif

START_NAMESPACE_STIR
class CListRecord;
class Succeeded;

class CListModeData
{
public:
  //! Use this type for save/set_get_position
  typedef unsigned int SavedPosition;

  //! Attempts to get a CListModeData object from a file
  static CListModeData* read_from_file(const string& filename);

  //! Default constructor
  /*! Initialises num_saved_get_positions to 0 */
  CListModeData();

  virtual
    ~CListModeData();

  //! Gets the next record in the listmode sequence
  virtual 
    Succeeded get_next_record(CListRecord& event) const = 0;

  //! Call this function if you want to re-start reading at the beginning.
  virtual 
    Succeeded reset() = 0;

  //! Save the current reading position
  /*! \warning There is a maximum number of times this function will be called.
      This is determined by the SavedPosition type. Once you save more
      positions, the first positions will be overwritten.
      \warning These saved positions are only valid for the lifetime of the 
      CListModeData object.
  */
  virtual
    SavedPosition save_get_position() = 0;

  //! Set the position for reading to a previously saved point
  virtual
    Succeeded set_get_position(const SavedPosition&) = 0;

  //! Get scanner pointer  
  virtual const Scanner* get_scanner_ptr() const;
  
protected:
  shared_ptr<Scanner> scanner_ptr;
  unsigned int num_saved_get_positions;
};

END_NAMESPACE_STIR

#endif
