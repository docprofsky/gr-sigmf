/* -*- c++ -*- */
/* 
 * Copyright 2018 gr-sigmf author.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_SIGMF_EXTRACT_ANNOTATION_IMPL_H
#define INCLUDED_SIGMF_EXTRACT_ANNOTATION_IMPL_H

#include <gnuradio/blocks/rotator.h>
#include <sigmf/extract_annotation.h>

namespace gr {
  namespace sigmf {

    class extract_annotation_impl : public extract_annotation
    {
      private:
      bool d_retune;
      double d_samp_rate;
      double d_center_freq;
      size_t d_samps_remaining;
      blocks::rotator d_rotator;

      const double TAU = 2*M_PI;

      void update_stream_state(size_t item_count);
      void set_rotator_freq(double abs_freq);

      public:
      extract_annotation_impl(bool retune);
      ~extract_annotation_impl();

      int general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items);
    };

  } // namespace sigmf
} // namespace gr

#endif /* INCLUDED_SIGMF_EXTRACT_ANNOTATION_IMPL_H */

