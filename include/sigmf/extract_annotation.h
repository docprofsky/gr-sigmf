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


#ifndef INCLUDED_SIGMF_EXTRACT_ANNOTATION_H
#define INCLUDED_SIGMF_EXTRACT_ANNOTATION_H

#include <sigmf/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace sigmf {

    /*!
     * \brief <+description of block+>
     * \ingroup sigmf
     *
     */
    class SIGMF_API extract_annotation : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<extract_annotation> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of sigmf::extract_annotation.
       *
       * To avoid accidental use of raw pointers, sigmf::extract_annotation's
       * constructor is in a private implementation
       * class. sigmf::extract_annotation::make is the public interface for
       * creating new instances.
       */
      static sptr make(bool retune);
    };

  } // namespace sigmf
} // namespace gr

#endif /* INCLUDED_SIGMF_EXTRACT_ANNOTATION_H */

