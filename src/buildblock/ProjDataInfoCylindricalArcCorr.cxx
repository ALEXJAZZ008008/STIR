//
// $Id$
//
/*!

  \file
  \ingroup projdata

  \brief Implementation of non-inline functions of class 
  ProjDataInfoCylindricalArcCorr

  \author Sanida Mustafovic
  \author Kris Thielemans
  \author PARAPET project

  $Date$

  $Revision$
*/
/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000- $Date$, Hammersmith Imanet Ltd
    See STIR/LICENSE.txt for details
*/

#include "stir/ProjDataInfoCylindricalArcCorr.h"
#include "stir/Bin.h"
#include "stir/round.h"
#include "stir/LORCoordinates.h"
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream.h>
#else
#include <sstream>
#endif

#ifndef STIR_NO_NAMESPACES
using std::endl;
using std::ends;
#endif


START_NAMESPACE_STIR
ProjDataInfoCylindricalArcCorr:: ProjDataInfoCylindricalArcCorr()
{}

ProjDataInfoCylindricalArcCorr:: ProjDataInfoCylindricalArcCorr(const shared_ptr<Scanner> scanner_ptr,float bin_size_v,								
								const  VectorWithOffset<int>& num_axial_pos_per_segment,
								const  VectorWithOffset<int>& min_ring_diff_v, 
								const  VectorWithOffset<int>& max_ring_diff_v,
								const int num_views,const int num_tangential_poss)
								:ProjDataInfoCylindrical(scanner_ptr,
								num_axial_pos_per_segment,
								min_ring_diff_v, max_ring_diff_v,
								num_views, num_tangential_poss),
								bin_size(bin_size_v)								
								
{}


void
ProjDataInfoCylindricalArcCorr::set_tangential_sampling(const float new_tangential_sampling)
{bin_size = new_tangential_sampling;}



ProjDataInfo*
ProjDataInfoCylindricalArcCorr::clone() const
{
  return static_cast<ProjDataInfo*>(new ProjDataInfoCylindricalArcCorr(*this));
}

string
ProjDataInfoCylindricalArcCorr::parameter_info()  const
{

#ifdef BOOST_NO_STRINGSTREAM
  // dangerous for out-of-range, but 'old-style' ostrstream seems to need this
  char str[50000];
  ostrstream s(str, 50000);
#else
  std::ostringstream s;
#endif  
  s << "ProjDataInfoCylindricalArcCorr := \n";
  s << ProjDataInfoCylindrical::parameter_info();
  s << "tangential sampling := " << get_tangential_sampling() << endl;
  s << "End :=\n";
  return s.str();
}


Bin
ProjDataInfoCylindricalArcCorr::
get_bin(const LOR<float>& lor) const

{
  Bin bin;
  LORInAxialAndSinogramCoordinates<float> lor_coords;
  if (lor.change_representation(lor_coords, get_ring_radius()) == Succeeded::no)
    {
      bin.set_bin_value(-1);
      return bin;
    }

  // first find view 
  // unfortunately, phi ranges from [0,Pi[, but the rounding can
  // map this to a view which corresponds to Pi anyway.
  bin.view_num() = round(lor_coords.phi() / get_azimuthal_angle_sampling());
  assert(bin.view_num()>=0);
  assert(bin.view_num()<=get_num_views());
  const bool swap_direction =
    bin.view_num() > get_max_view_num();

  int ring1, ring2;
  if (!swap_direction)
    {
      ring1 = round(lor_coords.z1()/get_ring_spacing());
      ring2 = round(lor_coords.z2()/get_ring_spacing());
    }
  else
    {
      ring2 = round(lor_coords.z1()/get_ring_spacing());
      ring1 = round(lor_coords.z2()/get_ring_spacing());
    }

  if (!(ring1 >=0 && ring1<get_scanner_ptr()->get_num_rings() &&
	ring2 >=0 && ring2<get_scanner_ptr()->get_num_rings() &&
	get_segment_axial_pos_num_for_ring_pair(bin.segment_num(),
						bin.axial_pos_num(),
						ring1,
						ring2) == Succeeded::yes)
      )
    {
      bin.set_bin_value(-1);
      return bin;
    }

  bin.tangential_pos_num() = round(lor_coords.s() / get_tangential_sampling());
  if (swap_direction)
    bin.tangential_pos_num() *= -1;

  if (bin.tangential_pos_num() < get_min_tangential_pos_num() ||
      bin.tangential_pos_num() > get_max_tangential_pos_num())
    {
      bin.set_bin_value(-1);
    }

  return bin;
}
END_NAMESPACE_STIR

