//
// $Id$
//
/*!
  \file
  \ingroup evaluation

  \brief Implementation of functions declared in tomo/evaluation/compute_ROI_values.h

  \author Kris Thielemans
  \author Damiano Belluzzo
  $Date$
  $Revision$
*/
/*
    Copyright (C) 2000- $Date$, Hammersmith Imanet Ltd
    See STIR/LICENSE.txt for details
*/
#include "stir/evaluation/compute_ROI_values.h"
#include "stir/Shape/Shape3D.h"
#include "stir/CartesianCoordinate2D.h"
#include "stir/CartesianCoordinate3D.h"
#include "stir/VoxelsOnCartesianGrid.h"
#include <numeric>


START_NAMESPACE_STIR

void
compute_ROI_values_per_plane(VectorWithOffset<ROIValues>& values, 
			     const DiscretisedDensity<3,float>& density, 
                             const Shape3D& shape,
                             const CartesianCoordinate3D<int>& num_samples)
{
  const VoxelsOnCartesianGrid<float>& image =
     dynamic_cast<const VoxelsOnCartesianGrid<float>&>(density);

  const int min_z = image.get_min_index();
  const int max_z = image.get_max_index();
  const CartesianCoordinate3D<float> voxel_size = image.get_voxel_size();
  const float voxel_volume = voxel_size.x() * voxel_size.y() * voxel_size.z();

  // initialise correct size
  values = VectorWithOffset<ROIValues>(min_z, max_z);

  
  // KT 14/11/2000 made pointer to prevent memory leak
  VoxelsOnCartesianGrid<float> *
    discretised_shape_ptr=image.get_empty_voxels_on_cartesian_grid();


  shape.construct_volume(*discretised_shape_ptr, num_samples);

  for (int z=min_z; z<=max_z; z++)
  {
    const float volume = (*discretised_shape_ptr)[z].sum() * voxel_volume;
    (*discretised_shape_ptr)[z] *= image[z];
    // TODO incorrect: picks up the values outside the ROI, which are 0
    const float ROI_min = (*discretised_shape_ptr)[z].find_min();
    const float ROI_max = (*discretised_shape_ptr)[z].find_max();
    const float integral = (*discretised_shape_ptr)[z].sum() * voxel_volume;
    (*discretised_shape_ptr)[z] *= image[z];
    const float integral_square =(*discretised_shape_ptr)[z].sum() * voxel_volume;
    values[z] =
      ROIValues(volume, integral, integral_square, 
                ROI_min, ROI_max);
  }
  delete discretised_shape_ptr;
}

ROIValues
compute_total_ROI_values(const VectorWithOffset<ROIValues>& values)
{
  // can't use std::accumulate, or we have to define ROIValues::operator+
  ROIValues tmp;
  for(VectorWithOffset<ROIValues>::const_iterator iter = values.begin();
      iter != values.end();
      iter++)
    tmp += *iter;
  return tmp;
}

ROIValues
compute_total_ROI_values(const DiscretisedDensity<3,float>& image,
                         const Shape3D& shape, 
                         const CartesianCoordinate3D<int>& num_samples
			 )
{
  VectorWithOffset<ROIValues> values;
  compute_ROI_values_per_plane(values, image, shape, num_samples);
  return 
    compute_total_ROI_values(values);
}




// Function that calculate the totals over a certain plane-range
 


void
compute_plane_range_ROI_values_per_plane(VectorWithOffset<ROIValues>& values, 
		  	           const DiscretisedDensity<3,float>& density,
			           const CartesianCoordinate2D<int>& plane_range,
                                   const Shape3D& shape,
                                   const CartesianCoordinate3D<int>& num_samples)
{
  const VoxelsOnCartesianGrid<float>& image =
     dynamic_cast<const VoxelsOnCartesianGrid<float>&>(density);

  const int min_z = image.get_min_index();
  const int max_z = image.get_max_index();
  // new range as entered, e.g end planes ignored

 int min_z_new = plane_range.x() - min_z;
 int max_z_new = max_z - plane_range.y();
  
  const CartesianCoordinate3D<float> voxel_size = image.get_voxel_size();
  const float voxel_volume = voxel_size.x() * voxel_size.y() * voxel_size.z();

  // initialise correct size
  values = VectorWithOffset<ROIValues>(min_z_new, max_z_new);

  VoxelsOnCartesianGrid<float> discretised_shape = 
    (*image.get_empty_voxels_on_cartesian_grid());

  shape.construct_volume(discretised_shape, num_samples);

  for (int z=min_z_new; z<=max_z_new; z++)
  {
    const float volume = discretised_shape.sum() * voxel_volume;
    discretised_shape[z] *= image[z];
    const float ROI_min = discretised_shape.find_min();
    const float ROI_max = discretised_shape.find_max();
    const float integral = discretised_shape.sum() * voxel_volume;
    discretised_shape[z] *= image[z];
    const float integral_square = discretised_shape.sum() * voxel_volume;
    values[z] =
      ROIValues(volume, integral, integral_square, 
                ROI_min, ROI_max);
  }
}


float
compute_CR_hot(ROIValues& val1, ROIValues& val2)

{
    return 1 - val1.get_mean()/val2.get_mean();
}

float
compute_CR_cold(ROIValues& val1, ROIValues& val2)
{
  return val1.get_mean()/val2.get_mean()- 1;
}


float
compute_uniformity(ROIValues& val)
{
  return val.get_stddev()/val.get_mean();
}

VectorWithOffset<float>
compute_CR_hot_per_plane(VectorWithOffset<ROIValues>& val1,VectorWithOffset<ROIValues>& val2)
{

 assert(val1.get_min_index()==val2.get_min_index());
 assert(val1.get_max_index()==val2.get_max_index());
  {

  VectorWithOffset<float> temp(val1.get_min_index(), val1.get_max_index()); 
    for (int i =val1.get_min_index();i<=val1.get_max_index();i++)

    {
      temp[i]= compute_CR_hot(val1[i],val2[i]);
    }
    return temp;
  }
}
VectorWithOffset<float>
compute_CR_cold_per_plane(VectorWithOffset<ROIValues>& val1,VectorWithOffset<ROIValues>& val2)
{

  assert(val1.get_min_index()==val2.get_min_index());
  assert(val1.get_max_index()==val2.get_max_index());
  {
    VectorWithOffset<float> temp(val1.get_min_index(), val1.get_max_index());
    for (int i =val1.get_min_index();i<=val1.get_max_index();i++)

    {
      temp[i] = compute_CR_cold(val1[i],val2[i]);
    }
    return temp;
  }

}
VectorWithOffset<float>
compute_uniformity_per_plane(VectorWithOffset<ROIValues>& val)
{

 VectorWithOffset<float> temp(val.get_min_index(), val.get_max_index());
  for (int i =val.get_min_index();i<=val.get_max_index();i++)
  {
    temp[i] = compute_uniformity(val[i]);
  }
  return temp;
}


END_NAMESPACE_STIR
