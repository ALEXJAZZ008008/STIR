//
// $Id$
//
/*!
  \file
  \ingroup ClearPET_utilities
  \brief Declaration of class CListModeDataLMF
    
  \author Kris Thielemans
      
  $Date$
  $Revision$
*/
/*
    Copyright (C) 2003- $Date$, IRSL
    See STIR/LICENSE.txt for details
*/

#ifndef __stir_listmode_CListModeDataLMF_H__
#define __stir_listmode_CListModeDataLMF_H__

#include "stir/listmode/CListModeData.h"
#include "stir/shared_ptr.h"
#include "lmf.h" // TODO adjust location
//#include "local/stir/ClearPET/LMF_ClearPET.h" // TODO don't know which is needed
//#include "local/stir/ClearPET/LMF_Interfile.h" 

#include <stdio.h>
#include <string>
#include <vector>

#ifndef STIR_NO_NAMESPACES
using std::string;
using std::vector;
#endif

START_NAMESPACE_STIR


//! A class that reads the listmode data from an LMF file
class CListModeDataLMF : public CListModeData
{
public:

  //! Constructor taking a filename
  CListModeDataLMF(const string& listmode_filename);

  // Destructor closes the file and destroys various structures
  ~CListModeDataLMF();

  virtual 
    shared_ptr <CListRecord> get_empty_record_sptr() const;


  //! LMF listmode data stores delayed events as well (as opposed to prompts)
  virtual bool has_delayeds() const 
    { return true; } // TODO always?

  virtual 
    Succeeded get_next_record(CListRecord& event) const;

  virtual 
    Succeeded reset();

  virtual 
    SavedPosition save_get_position();

  virtual 
    Succeeded set_get_position(const SavedPosition&);

private:

  string listmode_filename;
  // TODO we really want to make this a shared_ptr I think to avoid memory leaks when throwing exceptions
  struct LMF_ccs_encodingHeader *pEncoH;
  FILE *pfCCS;                                

  // possibly use this from LMF2Projection
  // SCANNER_CHECK_LIST scanCheckList;
  vector<unsigned long> saved_get_positions;

};

END_NAMESPACE_STIR

#endif
