//
// $Id$
//
/*!
  \file 
  \ingroup utilities

  \brief Program to compute fansums directly from listmode data
 
  \author Kris Thielemans
  
  $Date$
  $Revision $
*/
/*
    Copyright (C) 2002- $Date$, IRSL
    See STIR/LICENSE.txt for details
*/

/* Possible compilation switches:
  
HIDACREBINNER: 
  Enable code specific for the HiDAC
INCLUDE_NORMALISATION_FACTORS: 
  Enable code to include normalisation factors while rebinning.
  Currently only available for the HiDAC.
*/   

//#define HIDACREBINNER   
//#define INCLUDE_NORMALISATION_FACTORS


#include "stir/utilities.h"
#include "stir/shared_ptr.h"
#include "stir/ParsingObject.h"
#ifdef HIDACREBINNER
#include "local/stir/QHidac/lm_qhidac.h"

#else
#include "local/stir/listmode/lm.h"

#endif
#include "local/stir/listmode/CListModeData.h"
#include "stir/Scanner.h"
#include "stir/Array.h"
#include "stir/IndexRange2D.h"
#include "stir/stream.h"
#include "stir/CPUTimer.h"

#include "stir/ProjDataInfoCylindricalNoArcCorr.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <utility>

#ifndef STIR_NO_NAMESPACES
using std::fstream;
using std::ifstream;
using std::ofstream;
using std::cerr;
using std::cout;
using std::flush;
using std::endl;
using std::min;
using std::max;
using std::pair;
using std::vector;
using std::make_pair;
#endif

START_NAMESPACE_STIR

class TimeFrameDefinitions
{
public:
  TimeFrameDefinitions();
  explicit TimeFrameDefinitions(const string& fdef_filename);
  
  //! 1 based
  double get_start_time(unsigned int frame_num) const;
  double get_end_time(unsigned int frame_num) const;
  
  double get_start_time() const;
  double get_end_time() const;
  
  unsigned int get_num_frames() const;
  
private:
  vector<pair<double, double> > frame_times;
};

double
TimeFrameDefinitions::
get_start_time(unsigned int frame_num) const
{
  assert(frame_num>=1);
  assert(frame_num<=get_num_frames());
  return frame_times[frame_num-1].first;
}

double
TimeFrameDefinitions::
get_end_time(unsigned int frame_num) const
{
  assert(frame_num>=1);
  assert(frame_num<=get_num_frames());
  return frame_times[frame_num-1].second;
}

double
TimeFrameDefinitions::
get_start_time() const
{
  return get_start_time(1);
}

double
TimeFrameDefinitions::
get_end_time() const
{
  return get_end_time(get_num_frames());
}

unsigned int
TimeFrameDefinitions::
get_num_frames() const
{
  return frame_times.size();
}

TimeFrameDefinitions::
TimeFrameDefinitions()
{}

TimeFrameDefinitions::
TimeFrameDefinitions(const string& fdef_filename)
{
  ifstream in(fdef_filename.c_str());
  if (!in)
    error("Error reading %s\n", fdef_filename.c_str());

  
  double previous_end_time = 0;
  while (true)
  {
    int num;
    double duration;
    in >> num >> duration;
    if (!in || in.eof())
      break;
    if (num<=0 || duration<=0)
        error("Reading frame_def file %s: encountered negative numbers (%d, %g)\n",
	      fdef_filename.c_str(), num, duration);

    while (num--)
    {
      frame_times.push_back(make_pair(previous_end_time, previous_end_time+duration));
      previous_end_time+=duration;
    }
  }

  cerr << "Frame definitions:\n{";
  for (unsigned frame_num=1; frame_num<=get_num_frames(); ++frame_num)
  {
    cerr << '{' << get_start_time(frame_num) 
         << ',' << get_end_time(frame_num) 
         << '}';
    if (frame_num<get_num_frames())
      cerr << ',';
  }
  cerr << '}' << endl;
}

class LmFansums : public ParsingObject
{
public:

  LmFansums(const char * const par_filename);

  int max_segment_num_to_process;
  int fan_size;
  shared_ptr<CListModeData> lm_data_ptr;
  TimeFrameDefinitions frame_defs;

  void compute();
private:

  virtual void set_defaults();
  virtual void initialise_keymap();
  virtual bool post_processing();
  string input_filename;
  string output_filename_prefix;
  string frame_definition_filename;
  bool store_prompts;
int delayed_increment;
  int current_frame;

  bool interactive;

  void write_fan_sums(const Array<2,float>& data_fan_sums, 
               const unsigned current_frame_num) const;
};

void 
LmFansums::
set_defaults()
{
  max_segment_num_to_process = -1;
  fan_size = -1;
  store_prompts = true;
  delayed_increment = -1;
  interactive=false;
}

void 
LmFansums::
initialise_keymap()
{
  parser.add_start_key("lm_fansums Parameters");
  parser.add_key("input file",&input_filename);
  parser.add_key("frame_definition file",&frame_definition_filename);
  parser.add_key("output filename prefix",&output_filename_prefix);
  parser.add_key("tangential fan_size", &fan_size);
  parser.add_key("maximum absolute segment number to process", &max_segment_num_to_process); 

  if (CListEvent::has_delayeds())
  {
    parser.add_key("Store 'prompts'",&store_prompts);
    parser.add_key("increment to use for 'delayeds'",&delayed_increment);
  }
  parser.add_key("List event coordinates",&interactive);
  parser.add_stop_key("END");

#ifdef HIDACREBINNER     
  const unsigned int max_converter = 
    ask_num("Maximum allowed converter",0,15,15);
#endif
  

}


bool
LmFansums::
post_processing()
{
#ifdef HIDACREBINNER
  unsigned long input_file_offset = 0;
  LM_DATA_INFO lm_infos;
  read_lm_QHiDAC_data_head_only(&lm_infos,&input_file_offset,input_filename);
  lm_data_ptr =
    new CListModeDataFromStream(input_filename, input_file_offset);
#else
  // something similar will be done for other listmode types. TODO
  lm_data_ptr =
    CListModeData::read_from_file(input_filename);
#endif

  const int num_rings =
      lm_data_ptr->get_scanner_ptr()->get_num_rings();
  if (max_segment_num_to_process==-1)
    max_segment_num_to_process = num_rings-1;
  else
    max_segment_num_to_process =
      min(max_segment_num_to_process, num_rings-1);

  const int max_fan_size = 
    lm_data_ptr->get_scanner_ptr()->get_max_num_non_arccorrected_bins();
  if (fan_size==-1)
    fan_size = max_fan_size;
  else
    fan_size =
      min(fan_size, max_fan_size);

  frame_defs = TimeFrameDefinitions(frame_definition_filename);
  return false;
}

LmFansums::
LmFansums(const char * const par_filename)
{
  set_defaults();
  if (par_filename!=0)
    parse(par_filename) ;
  else
    ask_parameters();

}


void
LmFansums::
compute()
{

  //*********** get Scanner details
  shared_ptr<Scanner> scanner_ptr =
    new Scanner(*lm_data_ptr->get_scanner_ptr());
  const int num_rings = 
    lm_data_ptr->get_scanner_ptr()->get_num_rings();
  const int num_detectors_per_ring = 
    lm_data_ptr->get_scanner_ptr()->get_num_detectors_per_ring();
  


  const shared_ptr<ProjDataInfoCylindricalNoArcCorr> proj_data_info_ptr =
    dynamic_cast<ProjDataInfoCylindricalNoArcCorr *>
    (ProjDataInfo::ProjDataInfoCTI(scanner_ptr, 1, max_segment_num_to_process,
                                   scanner_ptr->get_num_detectors_per_ring()/2,
                                   fan_size,
                                   false));
  //*********** Finally, do the real work
  
  CPUTimer timer;
  timer.start();
    
  double time_of_last_stored_event = 0;
  long num_stored_events = 0;
  Array<2,float> data_fan_sums(IndexRange2D(num_rings, num_detectors_per_ring));
  
  // go to the beginning of the binary data
  lm_data_ptr->reset();
  
  unsigned int current_frame_num = 1;
  {      
    // loop over all events in the listmode file
    CListRecord record;
    double current_time = 0;
    while (true)
      {
        if (lm_data_ptr->get_next_record(record) == Succeeded::no) 
        {
          // no more events in file for some reason
          write_fan_sums(data_fan_sums, current_frame_num);
          break; //get out of while loop
        }
        if (record.is_time())
        {
          const double new_time = record.time.get_time_in_secs();
          if (new_time >= frame_defs.get_end_time(current_frame_num))
          {
            while (current_frame_num <= frame_defs.get_num_frames() &&
              new_time >= frame_defs.get_end_time(current_frame_num))
            {
              write_fan_sums(data_fan_sums, current_frame_num++);
              data_fan_sums.fill(0);
            }
            if (current_frame_num > frame_defs.get_num_frames())
              break; // get out of while loop
          }
          current_time = new_time;
        }
        else if (record.is_event() && frame_defs.get_start_time(current_frame_num) <= current_time)
	  {
            // see if we increment or decrement the value in the sinogram
            const int event_increment =
              record.event.is_prompt() 
              ? ( store_prompts ? 1 : 0 ) // it's a prompt
              :  delayed_increment;//it is a delayed-coincidence event
            
            if (event_increment==0)
              continue;
            

#ifdef HIDACREBINNER
	    if (record.event.conver_1 > max_converter ||
		record.event.conver_2 > max_converter) 
	      continue;
#error dont know how to do this
#else
            int ra,a,rb,b;
	    record.event.get_detectors(a,b,ra,rb);
	    if (abs(ra-rb)<=max_segment_num_to_process)
	      {
		const int det_num_diff =
		  (a-b+3*num_detectors_per_ring/2)%num_detectors_per_ring;
		if (det_num_diff<=fan_size/2 || 
		    det_num_diff>=num_detectors_per_ring-fan_size/2)
		  {
                  if (interactive)
                  {
                    printf("%c ra=%3d a=%4d, rb=%3d b=%4d, time=%8g accepted\n",
                      record.event.is_prompt() ? 'p' : 'd',
                      ra,a,rb,b,
                      current_time);
                    
                    Bin bin;
                    proj_data_info_ptr->get_bin_for_det_pair(bin,a, ra, b, rb);
                    printf("Seg %4d view %4d ax_pos %4d tang_pos %4d\n", 
                      bin.segment_num(), bin.view_num(), bin.axial_pos_num(), bin.tangential_pos_num());
                  }
		    data_fan_sums[ra][a] += event_increment;
		    data_fan_sums[rb][b] += event_increment;
		    num_stored_events += event_increment;
		  }
		else
		  {
#if 0
                    if (interactive)		  
		      printf(" ignored\n");
#endif
		  }
	      }
	    else
	      {
#if 0
              if (interactive)		  
		  printf(" ignored\n");
#endif
	      }
	    // TODO handle do_normalisation
#endif
	    
	  } // end of spatial event processing
      } // end of while loop over all events

    time_of_last_stored_event = 
      max(time_of_last_stored_event,current_time); 
  }


  timer.stop();
  
  cerr << "Last stored event was recorded after time-tick at " << time_of_last_stored_event << " secs\n";
  if (current_frame_num <= frame_defs.get_num_frames())
    cerr << "Early stop due to EOF. " << endl;
  cerr << "Total number of prompts/trues/delayed stored: " << num_stored_events << endl;

  cerr << "\nThis took " << timer.value() << "s CPU time." << endl;
}


// write fan sums to file
void 
LmFansums::
write_fan_sums(const Array<2,float>& data_fan_sums, 
               const unsigned current_frame_num) const
{
  char txt[50];
  sprintf(txt, "_f%d.dat", current_frame_num);
  string filename = output_filename_prefix;
  filename += txt;
  ofstream out(filename.c_str());
  out << data_fan_sums;
}

END_NAMESPACE_STIR


USING_NAMESPACE_STIR





/************************ main ************************/


int main(int argc, char * argv[])
{
  
  if (argc!=1 && argc!=2) {
    cerr << "Usage: " << argv[0] << " [par_file]\n";
    exit(EXIT_FAILURE);
  }
  LmFansums lm_fansums(argc==2 ? argv[1] : 0);
  lm_fansums.compute();

  return EXIT_SUCCESS;
}



