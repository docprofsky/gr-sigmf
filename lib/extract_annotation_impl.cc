/* -*- c++ -*- */
/* 
 * Copyright 2018 Scott Torborg
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "extract_annotation_impl.h"

namespace gr {
  namespace sigmf {

    extract_annotation::sptr
    extract_annotation::make(bool retune)
    {
      return gnuradio::get_initial_sptr
        (new extract_annotation_impl(retune));
    }

    /*
     * The private constructor
     */
    extract_annotation_impl::extract_annotation_impl(bool retune)
      : gr::block("extract_annotation",
                  gr::io_signature::make(1, 1, sizeof(gr_complex)),
                  gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_retune(retune), d_samp_rate(1.0), d_center_freq(0.0), d_samps_remaining(0)
    {
      // For now just drop all tags.
      set_tag_propagation_policy(TPP_DONT);

    }

    /*
     * Our virtual destructor.
     */
    extract_annotation_impl::~extract_annotation_impl()
    {
    }

    void
    extract_annotation_impl::update_stream_state(size_t item_count)
    {
      //std::cout << "updating stream state to item count " << item_count << std::endl;
      std::vector<tag_t> tags;
      get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + item_count);
      for(auto tag : tags) {
        std::cout << "got tag " << tag.key << ": " << tag.value << std::endl;
        if(pmt::equal(tag.key, pmt::intern("rx_freq"))) {
          d_center_freq = pmt::to_double(tag.value);
        } else if(pmt::equal(tag.key, pmt::intern("rx_rate"))) {
          d_samp_rate = pmt::to_double(tag.value);
        }
      }
    }

    void
    extract_annotation_impl::set_rotator_freq(double abs_freq)
    {
      double rel_freq = abs_freq - d_center_freq;
      std::cout << "setting rotator freq to " << rel_freq << std::endl;
      double q = ((TAU * (-1 * rel_freq)) / d_samp_rate);
      d_rotator.set_phase_incr(exp(gr_complex(0, q)));
    }

    int
    extract_annotation_impl::general_work(int noutput_items,
                                          gr_vector_int &ninput_items,
                                          gr_vector_const_void_star &input_items,
                                          gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      size_t input_count = (size_t) noutput_items;

      size_t consume_count = 0;
      size_t emit_count = 0;

      if(d_samps_remaining == 0) {
        // Not inside an annotation, so look for new annotations.
        //std::cout << "not inside an annotation, reading " << nitems_read(0) << " to " << nitems_read(0) + input_count << std::endl;

        std::vector<tag_t> tags;
        get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + input_count, pmt::intern("core:sample_count"));
        if(tags.size() > 0) {
          // Got a SigMF annotation, look for other associated tags.
          d_samps_remaining = pmt::to_uint64(tags[0].value);
          consume_count = tags[0].offset - nitems_read(0);
          std::cout << "got sigmf sample count " << d_samps_remaining << std::endl;

          // Update rotator state to retune to this annotation's center frequency.
          double freq_lower = 0.0;
          double freq_upper = 0.0;
          get_tags_in_range(tags, 0, tags[0].offset, tags[0].offset + 1, pmt::intern("core:freq_lower_edge"));
          if(tags.size() > 0) {
            freq_lower = pmt::to_double(tags[0].value);
            std::cout << "freq_lower_edge " << freq_lower << std::endl;
          }
          get_tags_in_range(tags, 0, tags[0].offset, tags[0].offset + 1, pmt::intern("core:freq_upper_edge"));
          if(tags.size() > 0) {
            freq_upper = pmt::to_double(tags[0].value);
            std::cout << "freq_upper_edge " << freq_upper << std::endl;
          }
          if((freq_lower > 0.0) && (freq_upper > 0.0)) {
            double abs_freq = (freq_upper + freq_lower) / 2.0;
            set_rotator_freq(abs_freq);
          } else {
            set_rotator_freq(0.0);
          }
        } else {
          consume_count = input_count;
        }
        emit_count = 0;
        update_stream_state(consume_count);

      } else {
        // Inside an annotation, so keep emitting until the end.
        //std::cout << "inside annotation, samps remaining " << d_samps_remaining << std::endl;

        emit_count = std::min(input_count, d_samps_remaining);
        consume_count = emit_count;
        d_samps_remaining -= emit_count;

        if(d_retune) {
          // Rotate and copy into output buffer.
          d_rotator.rotateN(out, in, emit_count);
        } else {
          // Not retuning, just copy input to output unchanged.
          memcpy(out, in, emit_count * sizeof(gr_complex));
        }
      }

      //std::cout << "consuming " << consume_count << std::endl;
      consume_each(consume_count);

      //std::cout << "returning, emitting " << emit_count << std::endl;
      // Tell runtime system how many output items we produced.
      return emit_count;
    }

  } /* namespace sigmf */
} /* namespace gr */

