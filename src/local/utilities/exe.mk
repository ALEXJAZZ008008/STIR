#
# $Id$
#

dir:=local/utilities

$(dir)_SOURCES = \
	fillwithotherprojdata.cxx \
	compute_plane_rescale_factors.cxx \
	apply_plane_rescale_factors.cxx \
	prepare_projdata.cxx \
	normalizedbckproj.cxx \
	inverse_proj_data.cxx 	\
	find_ML_normfactors3D.cxx \
	apply_normfactors3D.cxx \
	create_normfactors3D.cxx \
	find_ML_singles_from_delayed.cxx \
	threshold_norm_data.cxx \
	zero_projdata_from_norm.cxx \
	attenuation_coefficients_to_projections.cxx \
	construct_randoms_from_singles.cxx \
	interpolate_blocks.cxx \
	set_blocks_to_value.cxx \
	shift_projdata_along_axis.cxx \
	remove_sinograms.cxx \
	fit_cylinder.cxx \
	rigid_object_transform_projdata.cxx \
	rigid_object_transform_image.cxx \
	find_maximum_in_image.cxx \
	remove_corrupted_sinograms.cxx \
	fwd_image_and_fill_missing_data.cxx \
	add_planes_to_image.cxx \
	find_sinogram_rescaling_factors.cxx \
	move_image.cxx \
        find_fwhm_in_image.cxx 

#	CoG.cxx \
#	make_cylinder.cxx 

ifeq ($(HAVE_LLN_MATRIX),1)
  # yes, the LLN files seem to be there, so we can compile 
  # ecat7 stuff as well
  $(dir)_SOURCES += change_mhead_file_type.cxx copy_ecat7_header.cxx
endif

include $(WORKSPACE)/exe.mk
