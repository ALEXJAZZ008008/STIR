//
// $Id$
//

/*!
  \file
  \ingroup utilities

  \brief A utility applying/undoing some corrections on projection data

  This is useful to precorrect projection data. There's also the option to undo
  the correction.

  Here's a sample .par file
\verbatim
correct_projdata Parameters := 
  input file := trues.hs

  ; Current way of specifying time frames, pending modifications to
  ; STIR to read time info from the headers
  ; see class documentation for TimeFrameDefinitions for the format of this file
  ; time frame definition filename :=  frames.fdef

  ; if a frame definition file is specified, you can say that the input data
  ; corresponds to a specific time frame
  ; the number should be between 1 and num_frames and defaults to 1
  ; this is currently only used to pass the relevant time to the normalisation
  ; time frame number := 1
  
  ; output file
  ; for future compatibility, do not use an extension in the name of 
  ; the output file. It will be added automatically
  output filename := precorrected

  ; default value for next is -1, meaning 'all segments'
  ; maximum absolute segment number to process := 
 

  ; use data in the input file, or substitute data with all 1's
  ; (useful to get correction factors only)
  ; default is '1'
  ; use data (1) or set to one (0) := 

  ; precorrect data, or undo precorrection
  ; default is '1'
  ; apply (1) or undo (0) correction := 

  ; parameters specifying correction factors
  ; if no value is given, the corresponding correction will not be performed

  ; random coincidences estimate, subtracted before anything else is done
  ;randoms projdata filename := random.hs
  ; normalisation (or binwise multiplication, so can contain attenuation factors as well)
  Bin Normalisation type := from projdata
    Bin Normalisation From ProjData :=
    normalisation projdata filename:= norm.hs
    End Bin Normalisation From ProjData:=

  ; attenuation image, will be forward projected to get attenuation factors
  ; OBSOLETE
  ;attenuation image filename := attenuation_image.hv
  
  ; forward projector used to estimate attenuation factors, defaults to Ray Tracing
  ; OBSOLETE
  ;forward_projector type := Ray Tracing

  ; scatter term to be subtracted AFTER norm+atten correction
  ; defaults to 0
  ;scatter projdata filename := scatter.hs
END:= 
\endverbatim

Time frame definition is only necessary when the normalisation type uses
this time info for dead-time correction.

The following gives a brief explanation of the non-obvious parameters. 

<ul>
<li> use data (1) or set to one (0):<br>
Use the data in the input file, or substitute data with all 1's. This is useful to get correction factors only. Its value defaults to 1.
</li>
<li>
apply (1) or undo (0) correction:<br>
Precorrect data, or undo precorrection. Its value defaults to 1.
</li>
<li>
Bin Normalisation type:<br>
Normalisation (or binwise multiplication, so can contain attenuation factors 
as well). \see BinNormalisation
</li>
<li>
attenuation image filename: obsolete<br>
Specify the attenuation image, which will be forward projected to get 
attenuation factors. Has to be in units cm^-1.
This parameter will be removed. Use instead a "chained" bin normalisation 
with a bin normalisation "from attenuation image" 
\see ChainedBinNormalisation
\see BinNormalisationFromAttenuationImage
</li>
<li>
forward_projector type: obsolete
Forward projector used to estimate attenuation factors, defaults to 
Ray Tracing. \see ForwardProjectorUsingRayTracing
This parameter will be removed.
</li>
</ul>

  \author Kris Thielemans

  $Date$
  $Revision$
*/
/*
    Copyright (C) 2000- $Date$, Hammersmith Imanet Ltd
    See STIR/LICENSE.txt for details
*/


#include "stir/utilities.h"
#include "stir/CPUTimer.h"
#include "stir/VoxelsOnCartesianGrid.h"
#include "stir/ProjDataInterfile.h"
#include "stir/RelatedViewgrams.h"
#include "stir/ParsingObject.h"
#include "stir/Succeeded.h"
#include "stir/recon_buildblock/TrivialBinNormalisation.h"
#include "stir/TrivialDataSymmetriesForViewSegmentNumbers.h"
#include "stir/ArrayFunction.h"
#include "stir/TimeFrameDefinitions.h"
#ifndef USE_PMRT
#include "stir/recon_buildblock/ForwardProjectorByBinUsingRayTracing.h"
#else
#include "stir/recon_buildblock/ForwardProjectorByBinUsingProjMatrixByBin.h"
#include "stir/recon_buildblock/ProjMatrixByBinUsingRayTracing.h"
#endif
#include "stir/is_null_ptr.h"
#include <string>
#include <iostream> 
#include <fstream>
#include <algorithm>

#ifndef STIR_NO_NAMESPACES
using std::cerr;
using std::endl;
using std::fstream;
using std::ifstream;
using std::cout;
using std::string;
#endif

START_NAMESPACE_STIR





// note: apply_or_undo_correction==true means: apply it
static void
correct_projection_data(ProjData& output_projdata, const ProjData& input_projdata,
			const bool use_data_or_set_to_1,
			const bool apply_or_undo_correction,
                        const shared_ptr<ProjData>& scatter_projdata_ptr,
			shared_ptr<DiscretisedDensity<3,float> >& attenuation_image_ptr,
			const shared_ptr<ForwardProjectorByBin>& forward_projector_ptr,
			BinNormalisation& normalisation,
                        const shared_ptr<ProjData>& randoms_projdata_ptr, 
			const int frame_num,
			const TimeFrameDefinitions& frame_def)
{

  const bool do_attenuation = attenuation_image_ptr.use_count() != 0;
  const bool do_scatter = scatter_projdata_ptr.use_count() != 0;
  const bool do_randoms = randoms_projdata_ptr.use_count() != 0;


  shared_ptr<DataSymmetriesForViewSegmentNumbers> symmetries_ptr = 
    !do_attenuation ?
      new TrivialDataSymmetriesForViewSegmentNumbers
    :
      forward_projector_ptr->get_symmetries_used()->clone();

  for (int segment_num = output_projdata.get_min_segment_num(); segment_num <= output_projdata.get_max_segment_num() ; segment_num++)
  {
    cerr<<endl<<"Processing segment # "<<segment_num << "(and any related segments)"<<endl;
    for (int view_num=input_projdata.get_min_view_num(); view_num<=input_projdata.get_max_view_num(); ++view_num)
    {    
      const ViewSegmentNumbers view_seg_nums(view_num,segment_num);
      if (!symmetries_ptr->is_basic(view_seg_nums))
        continue;
      
      // ** first fill in the data **
      
      RelatedViewgrams<float> 
        viewgrams = output_projdata.get_empty_related_viewgrams(ViewSegmentNumbers(view_num,segment_num),
	                          symmetries_ptr, false);
      if (use_data_or_set_to_1)
      {
	// Unfortunately, segment range in output_projdata and input_projdata can be
        // different. So, we cannot simply use 
        // viewgrams = input_projdata.get_related_viewgrams
        // as this would give it the wrong proj_data_info_ptr 
        // (resulting in problems when setting the viewgrams in output_projdata).
        // The trick relies on calling Array::operator+= instead of 
        // Viewgrams::operator=
        viewgrams += 
          input_projdata.get_related_viewgrams(view_seg_nums,
	                            symmetries_ptr, false);
      }	  
      else
      {
        viewgrams.fill(1.F);
      }
      
	      
      // display(viewgrams);      

      if (do_scatter && !apply_or_undo_correction)
      {
        viewgrams += 
          scatter_projdata_ptr->get_related_viewgrams(view_seg_nums,
	                                              symmetries_ptr, false);
      }

      if (do_randoms && apply_or_undo_correction)
      {
        viewgrams -= 
          randoms_projdata_ptr->get_related_viewgrams(view_seg_nums,
	                                              symmetries_ptr, false);
      }
#if 0
      if (frame_num==-1)
      {
	int num_frames = frame_def.get_num_frames();
	for ( int i = 1; i<=num_frames; i++)
	{ 
	  //cerr << "Doing frame  " << i << endl; 
	  const double start_frame = frame_def.get_start_time(i);
	  const double end_frame = frame_def.get_end_time(i);
	  //cerr << "Start time " << start_frame << endl;
	  //cerr << " End time " << end_frame << endl;
	  // ** normalisation **
	  if (apply_or_undo_correction)
	  {
	    normalisation.apply(viewgrams,start_frame,end_frame);
	  }
	  else
	  {
	    normalisation.undo(viewgrams,start_frame,end_frame);
	  }
	}
      }
      else
#endif
	{      
	const double start_frame = frame_def.get_start_time(frame_num);
	const double end_frame = frame_def.get_end_time(frame_num);
	if (apply_or_undo_correction)
	{
	  normalisation.apply(viewgrams,start_frame,end_frame);
	}
	else
	{
	  normalisation.undo(viewgrams,start_frame,end_frame);
	}    
     }
       

      // ** attenuation ** 
      if (do_attenuation)
      {	
	RelatedViewgrams<float> attenuation_viewgrams = 
	  output_projdata.get_empty_related_viewgrams(view_seg_nums,
	                                  symmetries_ptr, false);	
	
	forward_projector_ptr->forward_project(attenuation_viewgrams, *attenuation_image_ptr);
	
	// TODO cannot use std::transform ?
	for (RelatedViewgrams<float>::iterator viewgrams_iter = 
	             attenuation_viewgrams.begin();
	     viewgrams_iter != attenuation_viewgrams.end();
	     ++viewgrams_iter)
	{
	  in_place_exp(*viewgrams_iter);
	}
	if (apply_or_undo_correction)
          viewgrams *= attenuation_viewgrams;
        else
          viewgrams /= attenuation_viewgrams;

      } // do_attenuation
      

      if (do_scatter && apply_or_undo_correction)
      {
        viewgrams -= 
          scatter_projdata_ptr->get_related_viewgrams(view_seg_nums,
	                                              symmetries_ptr, false);
      }

      if (do_randoms && !apply_or_undo_correction)
      {
        viewgrams += 
          randoms_projdata_ptr->get_related_viewgrams(view_seg_nums,
	                                              symmetries_ptr, false);
      }
      
      if (!(output_projdata.set_related_viewgrams(viewgrams) == Succeeded::yes))
        error("Error set_related_viewgrams\n");            
      
    }
        
  }
}    

// TODO most of this is identical to ReconstructionParameters, so make a common class
class CorrectProjDataParameters : public ParsingObject
{
public:

  CorrectProjDataParameters(const char * const par_filename);

  // shared_ptrs such that they clean up automatically at exit
  shared_ptr<ProjData> input_projdata_ptr;
  shared_ptr<ProjData> scatter_projdata_ptr;
  shared_ptr<ProjData> randoms_projdata_ptr;
  shared_ptr<ProjData> output_projdata_ptr;
  shared_ptr<BinNormalisation> normalisation_ptr;
  shared_ptr<DiscretisedDensity<3,float> > attenuation_image_ptr;
  shared_ptr<ForwardProjectorByBin> forward_projector_ptr;
  bool apply_or_undo_correction;
  bool use_data_or_set_to_1;  
  int max_segment_num_to_process;
  int frame_num;
  TimeFrameDefinitions frame_defs;
  private:

  virtual void set_defaults();
  virtual void initialise_keymap();
  virtual bool post_processing();
  string input_filename;
  string output_filename;
  string scatter_projdata_filename;
  string atten_image_filename;
  string norm_filename;  
  string randoms_projdata_filename;
  
  string frame_definition_filename;
  
};

void 
CorrectProjDataParameters::
set_defaults()
{
  input_projdata_ptr = 0;
  max_segment_num_to_process = -1;
  normalisation_ptr = 0;
  use_data_or_set_to_1= true;
  apply_or_undo_correction = true;
  scatter_projdata_filename = "";
  atten_image_filename = "";
  norm_filename = "";
  normalisation_ptr = new TrivialBinNormalisation;
  randoms_projdata_filename = "";
  attenuation_image_ptr = 0;
  frame_num = 1;

#ifndef USE_PMRT
  forward_projector_ptr =
    new ForwardProjectorByBinUsingRayTracing;
#else
  shared_ptr<ProjMatrixByBin> PM = 
    new  ProjMatrixByBinUsingRayTracing;
  forward_projector_ptr =
    new ForwardProjectorByBinUsingProjMatrixByBin(PM); 
#endif
}

void 
CorrectProjDataParameters::
initialise_keymap()
{
  parser.add_start_key("correct_projdata Parameters");
  parser.add_key("input file",&input_filename);
  parser.add_key("time frame definition filename", &frame_definition_filename); 
  parser.add_key("time frame number", &frame_num);

  parser.add_key("output filename",&output_filename);
  parser.add_key("maximum absolute segment number to process", &max_segment_num_to_process);
 
  parser.add_key("use data (1) or set to one (0)", &use_data_or_set_to_1);
  parser.add_key("apply (1) or undo (0) correction", &apply_or_undo_correction);
  parser.add_parsing_key("Bin Normalisation type", &normalisation_ptr);
  parser.add_key("randoms projdata filename", &randoms_projdata_filename);
  parser.add_key("attenuation image filename", &atten_image_filename);
  parser.add_parsing_key("forward projector type", &forward_projector_ptr);
  parser.add_key("scatter_projdata_filename", &scatter_projdata_filename);
  parser.add_stop_key("END");
}


bool
CorrectProjDataParameters::
post_processing()
{
  if (is_null_ptr(normalisation_ptr))
  {
    warning("Invalid normalisation object\n");
    return true;
  }

  // read time frame def 
   if (frame_definition_filename.size()!=0)
    frame_defs = TimeFrameDefinitions(frame_definition_filename);
   else
    {
      // make a single frame starting from 0 to 1.
      vector<pair<double, double> > frame_times(1, pair<double,double>(0,1));
      frame_defs = TimeFrameDefinitions(frame_times);
    }

  if (frame_num<=0)
    {
      warning("frame_num should be >= 1 \n");
      return true;
    }

  if (static_cast<unsigned>(frame_num)> frame_defs.get_num_frames())
    {
      warning("frame_num is %d, but should be less than the number of frames %d.\n",
	      frame_num, frame_defs.get_num_frames());
      return true;
    }
  return false;
}

CorrectProjDataParameters::
CorrectProjDataParameters(const char * const par_filename)
{
  set_defaults();
  if (par_filename!=0)
    parse(par_filename) ;
  else
    ask_parameters();
  input_projdata_ptr = ProjData::read_from_file(input_filename);

  if (scatter_projdata_filename!="" && scatter_projdata_filename != "0")
    scatter_projdata_ptr = ProjData::read_from_file(scatter_projdata_filename);

  if (randoms_projdata_filename!="" && randoms_projdata_filename != "0")
    randoms_projdata_ptr = ProjData::read_from_file(randoms_projdata_filename);

  const int max_segment_num_available =
    input_projdata_ptr->get_max_segment_num();
  if (max_segment_num_to_process<0 ||
      max_segment_num_to_process > max_segment_num_available)
    max_segment_num_to_process = max_segment_num_available;
  shared_ptr<ProjDataInfo>  new_data_info_ptr= 
    input_projdata_ptr->get_proj_data_info_ptr()->clone();
    
  new_data_info_ptr->reduce_segment_range(-max_segment_num_to_process, 
					  max_segment_num_to_process);

  // construct output_projdata
  {
#if 0
    // attempt to do mult-frame data, but then we should have different input data anyway
    if (frame_definition_filename.size()!=0 && frame_num==-1)
      {
	const int num_frames = frame_defs.get_num_frames();
	for ( int current_frame = 1; current_frame <= num_frames; current_frame++)
	  {
	    char ext[50];
	    sprintf(ext, "_f%dg1b0d0", current_frame);
	    const string output_filename_with_ext = output_filename + ext;	
	    output_projdata_ptr = new ProjDataInterfile(new_data_info_ptr,output_filename_with_ext);
	  }
      }
    else
#endif
      {
	string output_filename_with_ext = output_filename;
#if 0
	if (frame_definition_filename.size()!=0)
	  {
	    char ext[50];
	    sprintf(ext, "_f%dg1b0d0", frame_num);
	    output_filename_with_ext += ext;
	  }
#endif
      output_projdata_ptr = new ProjDataInterfile(new_data_info_ptr,output_filename_with_ext);
      }

  }
 
  // set up normalisation object
  if (
      normalisation_ptr->set_up(output_projdata_ptr->get_proj_data_info_ptr()->clone())
      != Succeeded::yes)
    error("correct_projdata: set-up of normalisation failed\n");
 
  // read attenuation data
  if(atten_image_filename!="0" && atten_image_filename!="")
  {
    attenuation_image_ptr = 
      DiscretisedDensity<3,float>::read_from_file(atten_image_filename.c_str());
        
    cerr << "WARNING: attenuation image data are supposed to be in units cm^-1\n"
      "Reference: water has mu .096 cm^-1" << endl;
    cerr<< "Max in attenuation image:" 
      << attenuation_image_ptr->find_max() << endl;
#ifndef NORESCALE
    /*
      cerr << "WARNING: multiplying attenuation image by x-voxel size "
      << " to correct for scale factor in forward projectors...\n";
    */
    // projectors work in pixel units, so convert attenuation data 
    // from cm^-1 to pixel_units^-1
    const float rescale = 
      dynamic_cast<VoxelsOnCartesianGrid<float> *>(attenuation_image_ptr.get())->
      get_voxel_size().x()/10;
#else
    const float rescale = 
      10.F;
#endif
    *attenuation_image_ptr *= rescale;

    forward_projector_ptr->set_up(output_projdata_ptr->get_proj_data_info_ptr()->clone(),
				  attenuation_image_ptr);
  }

}


END_NAMESPACE_STIR

USING_NAMESPACE_STIR

int main(int argc, char *argv[])
{
  
  if(argc!=2) 
  {
    cerr<<"Usage: " << argv[0] << " par_file\n"
       	<< endl; 
  }
  CorrectProjDataParameters parameters( argc==2 ? argv[1] : 0);
 
  if (argc!=2)
    {
      cerr << "Corresponding .par file input \n"
	   << parameters.parameter_info() << endl;
    }
    

  CPUTimer timer;
  timer.start();

  correct_projection_data(*parameters.output_projdata_ptr, *parameters.input_projdata_ptr, 
			  parameters.use_data_or_set_to_1, parameters.apply_or_undo_correction,
                          parameters.scatter_projdata_ptr,
			  parameters.attenuation_image_ptr,  
			  parameters.forward_projector_ptr,  
			  *parameters.normalisation_ptr,
                          parameters.randoms_projdata_ptr,
			  parameters.frame_num,
			  parameters.frame_defs);
 
  timer.stop();
  cerr << "CPU time : " << timer.value() << "secs" << endl;
  return EXIT_SUCCESS;

}
