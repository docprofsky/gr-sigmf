/* -*- c++ -*- */
/* 
 * Copyright 2018 Scott Torborg, Paul Wicks, Caitlin Miller
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

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <uhd/usrp/multi_usrp.hpp>
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include "usrp_gps_message_source_impl.h"
#include <boost/thread.hpp>

namespace gr {
  namespace sigmf {

    double
    parse_nmea_latitude(const std::string raw, const std::string dir)
    {
      int degrees = std::stoi(raw.substr(0, 2));
      double minutes = std::stod(raw.substr(2));
      double out = (minutes / 60.0) + degrees;
      if(dir != "N") {
        out = -out;
      }
      return out;
    }

    double
    parse_nmea_longitude(const std::string raw, const std::string dir)
    {
      int degrees = std::stoi(raw.substr(0, 3));
      double minutes = std::stod(raw.substr(3));
      double out = (minutes / 60.0) + degrees;
      if(dir != "E") {
        out = -out;
      }
      return out;
    }

    usrp_gps_message_source::sptr
    usrp_gps_message_source::make(const ::uhd::device_addr_t &uhd_args, double poll_interval)
    {
      return gnuradio::get_initial_sptr
        (new usrp_gps_message_source_impl(uhd_args, poll_interval));
    }

    /*
     * The private constructor
     */
    usrp_gps_message_source_impl::usrp_gps_message_source_impl(const ::uhd::device_addr_t &uhd_args, double poll_interval)
      : gr::block("usrp_gps_message_source",
                  gr::io_signature::make(0, 0, 0),
                  gr::io_signature::make(0, 0, 0)),
      d_finished(false),
      d_poll_interval(poll_interval),
      d_mboard(0)
    {
      message_port_register_out(pmt::intern("out"));
      d_usrp = ::uhd::usrp::multi_usrp::make(uhd_args);
    }

    /*
     * Our virtual destructor.
     */
    usrp_gps_message_source_impl::~usrp_gps_message_source_impl()
    {
    }

    bool
    usrp_gps_message_source_impl::start()
    {
      d_finished = false;
      // Start the polling thread.
      d_poll_thread = gr::thread::thread(boost::bind(&usrp_gps_message_source_impl::poll_thread, this));
      return gr::block::start();
    }

    bool
    usrp_gps_message_source_impl::stop()
    {
      // Shut down the polling thread.
      d_finished = true;
      d_poll_thread.interrupt();
      d_poll_thread.join();
      return gr::block::stop();
    }

    pmt::pmt_t
    usrp_gps_message_source_impl::poll_now()
    {
      uint64_t gps_time = d_usrp->get_mboard_sensor("gps_time", d_mboard).to_int();
      bool gps_locked = d_usrp->get_mboard_sensor("gps_locked", d_mboard).to_bool();
      const std::string gps_gpgga = d_usrp->get_mboard_sensor("gps_gpgga", d_mboard).to_pp_string();

      // Tokenize gpgga sentence
      std::vector<std::string> tokens;
      std::stringstream stream_gpgga(gps_gpgga);
      std::string tok;
      while(std::getline(stream_gpgga, tok, ',')) {
        tokens.push_back(tok);
      }

      // Extract and parse fields
      double lat = parse_nmea_latitude(tokens[2], tokens[3]);
      double lon = parse_nmea_longitude(tokens[4], tokens[5]);

      int fix_quality = std::stoi(tokens[6]);
      int num_sats = std::stoi(tokens[7]);
      double hdop = std::stod(tokens[8]);
      double alt = std::stod(tokens[9]);

      // return a pmt which we can use to emit a message
      pmt::pmt_t values = pmt::make_dict();

      values = pmt::dict_add(values, pmt::intern("gps_time"), pmt::from_uint64(gps_time));
      values = pmt::dict_add(values, pmt::intern("gps_locked"), pmt::from_bool(gps_locked));
      values = pmt::dict_add(values, pmt::intern("latitude"), pmt::from_double(lat));
      values = pmt::dict_add(values, pmt::intern("longitude"), pmt::from_double(lon));
      values = pmt::dict_add(values, pmt::intern("altitude"), pmt::from_double(alt));
      values = pmt::dict_add(values, pmt::intern("fix_quality"), pmt::from_long(fix_quality));
      values = pmt::dict_add(values, pmt::intern("hdop"), pmt::from_double(hdop));
      values = pmt::dict_add(values, pmt::intern("num_sats"), pmt::from_long(num_sats));
      values = pmt::dict_add(values, pmt::intern("gps_gpgga"), pmt::string_to_symbol(gps_gpgga));

      return values;
    }

    void
    usrp_gps_message_source_impl::poll_thread()
    {
      while(true) {
        std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
        std::chrono::time_point<std::chrono::steady_clock> next_tick = now + std::chrono::milliseconds((int) (d_poll_interval * 1000.0));

        pmt::pmt_t values = poll_now();
        message_port_pub(pmt::intern("out"), values);

        std::this_thread::sleep_until(next_tick);
      }
    }
  } /* namespace sigmf */
} /* namespace gr */

