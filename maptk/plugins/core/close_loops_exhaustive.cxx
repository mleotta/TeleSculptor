/*ckwg +29
 * Copyright 2016 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * \brief Implementation of close_loops_exhaustive
 */

#include "close_loops_exhaustive.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <functional>

#include <vital/algo/algorithm.h>
#include <vital/exceptions/algorithm.h>


namespace kwiver {
namespace maptk {

namespace core
{

using namespace kwiver::vital;

/// Default Constructor
close_loops_exhaustive
::close_loops_exhaustive()
: enabled_(true),
  match_req_(100),
  num_look_back_(-1)
{
}


/// Copy Constructor
close_loops_exhaustive
::close_loops_exhaustive(const close_loops_exhaustive& other)
: enabled_(other.enabled_),
  match_req_(other.match_req_),
  num_look_back_(other.num_look_back_),
  matcher_(!other.matcher_ ? algo::match_features_sptr() : other.matcher_->clone())
{
}


std::string
close_loops_exhaustive
::description() const
{
  return "Attempts long-term loop closure";
}


/// Get this alg's \link vital::config_block configuration block \endlink
  vital::config_block_sptr
close_loops_exhaustive
::get_configuration() const
{
  // get base config from base class
  vital::config_block_sptr config = algorithm::get_configuration();

  // Sub-algorithm implementation name + sub_config block
  // - Feature Matcher algorithm
  algo::match_features::get_nested_algo_configuration("feature_matcher", config, matcher_);

  config->set_value("enabled", enabled_,
                    "Should exhaustive loop closure be enabled? This option will attempt to "
                    "bridge the gap between frames and previous frames exhaustively");

  config->set_value("match_req", match_req_,
                    "The required number of features needed to be matched for a success.");

  config->set_value("num_look_back", num_look_back_,
                    "Maximum number of frames to search in the past for matching to "
                    "(-1 looks back to the beginning).");

  return config;
}


/// Set this algo's properties via a config block
void
close_loops_exhaustive
::set_configuration(vital::config_block_sptr in_config)
{
  // Starting with our generated config_block to ensure that assumed values are present
  // An alternative is to check for key presence before performing a get_value() call.
  vital::config_block_sptr config = this->get_configuration();
  config->merge_config(in_config);

  // Setting nested algorithm instances via setter methods instead of directly
  // assigning to instance property.
  algo::match_features_sptr mf;
  algo::match_features::set_nested_algo_configuration("feature_matcher", config, mf);
  matcher_ = mf;

  enabled_ = config->get_value<bool>("enabled");
  match_req_ = config->get_value<int>("match_req");
  num_look_back_ = config->get_value<int>("num_look_back");
}


bool
close_loops_exhaustive
::check_configuration(vital::config_block_sptr config) const
{
  return (
    algo::match_features::check_nested_algo_configuration("feature_matcher", config)
  );
}


/// Functor to help remove tracks from vector
bool track_in_set( track_sptr trk_ptr, std::set<track_id_t>* set_ptr )
{
  return set_ptr->find( trk_ptr->id() ) != set_ptr->end();
}


/// Exaustive loop closure
vital::track_set_sptr
close_loops_exhaustive
::stitch( vital::frame_id_t frame_number,
          vital::track_set_sptr input,
          vital::image_container_sptr,
          vital::image_container_sptr ) const
{
  // check if enabled and possible
  if( !enabled_)
  {
    return input;
  }

  frame_id_t last_frame = 0;
  if (num_look_back_ >= 0)
  {
    last_frame = std::max<int>(frame_number - num_look_back_, 0);
  }

  std::vector< vital::track_sptr > all_tracks = input->tracks();
  vital::track_set_sptr current_set = input->active_tracks( frame_number );

  for(vital::frame_id_t f = frame_number - 2; f >= last_frame; f-- )
  {
    vital::track_set_sptr f_set = input->active_tracks( f );

    // run matcher alg
    vital::match_set_sptr mset = matcher_->match(f_set->frame_features( f ),
                                                 f_set->frame_descriptors( f ),
                                                 current_set->frame_features( frame_number ),
                                                 current_set->frame_descriptors( frame_number ));

    if( mset->size() < match_req_ )
    {
      continue;
    }

    // modify track history
    std::vector<vital::track_sptr> f_tracks = f_set->tracks();
    std::vector<vital::track_sptr> current_tracks = current_set->tracks();
    std::vector<vital::match> matches = mset->matches();
    std::set<vital::track_id_t> to_remove;

    for( unsigned i = 0; i < matches.size(); i++ )
    {
      if( f_tracks[ matches[i].first ]->append( *current_tracks[ matches[i].second ] ) )
      {
        to_remove.insert( current_tracks[ matches[i].second ]->id() );
      }
    }

    if( !to_remove.empty() )
    {
      all_tracks.erase(
        std::remove_if( all_tracks.begin(), all_tracks.end(),
                        std::bind( track_in_set, std::placeholders::_1, &to_remove ) ),
        all_tracks.end()
      );
    }
  }

  return track_set_sptr( new simple_track_set( all_tracks ) );
}


} // end namespace core

} // end namespace maptk
} // end namespace kwiver
