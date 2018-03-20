#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2017 Scott Torborg, Paul Wicks, Caitlin Miller
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

import datetime
from decimal import Decimal
from threading import Timer

import pmt
from gnuradio import gr


class usrp_gps_probe(gr.basic_block):
    """
    Queries a USRP block (source or sink) at regular intervals for GPS
    position. If a GPS sensor is available and has a position fix, emit a
    message containing the decoded position and time of the radio.
    """
    def __init__(self, usrp_block, interval, require_valid=True,
                 sensor='gps_gprmc'):
        gr.basic_block.__init__(self, name="usrp_gps_probe",
                                in_sig=None, out_sig=None)
        self.interval = interval
        self.require_valid = require_valid
        self.sensor = sensor
        self.usrp_block = usrp_block
        self.message_port_register_out(pmt.intern('out'))
        self.timer = None

    def reset_timer(self):
        self.timer = Timer(self.interval / 1000.0, self.probe)
        self.timer.start()

    def start(self):
        if self.sensor in self.usrp_block.get_mboard_sensor_names():
            self.reset_timer()
        return True

    def stop(self):
        if self.timer:
            self.timer.cancel()
        return True

    @classmethod
    def parse_nmea_latitude(cls, s, dir):
        degrees = Decimal(s[:2])
        minutes = Decimal(s[2:])
        degrees += (minutes / 60)
        if dir == 'S':
            degrees = -degrees
        return degrees

    @classmethod
    def parse_nmea_longitude(cls, s, dir):
        degrees = Decimal(s[:3])
        minutes = Decimal(s[3:])
        degrees += (minutes / 60)
        if dir == 'W':
            degrees = -degrees
        return degrees

    @classmethod
    def parse_nmea_timestamp(cls, utc_date, utc_time):
        utc_time, frac_secs = utc_time.rsplit('.', 1)
        utc_string = "%s-%s" % (utc_date, utc_time)
        full_secs = datetime.datetime.strptime(utc_string, '%d%m%y-%H%M%S')
        timestamp = ''.join([full_secs.isoformat(), '.', frac_secs, 'Z'])
        return timestamp

    @classmethod
    def parse_nmea_sentence(cls, sentence):
        assert sentence.startswith('$GPRMC')
        fields = sentence.split(',')
        utc_time = fields[1]  # HHMMSS.mm
        valid = fields[2] == 'A'

        lat = cls.parse_nmea_latitude(fields[3], fields[4])
        lon = cls.parse_nmea_longitude(fields[5], fields[6])

        utc_date = fields[9]  # as DDMMYY
        timestamp = cls.parse_nmea_timestamp(utc_date, utc_time)

        return {
            'valid': valid,
            'timestamp': timestamp,
            'latitude': str(lat.quantize(Decimal('.000001'))),
            'longitude': str(lon.quantize(Decimal('.000001'))),
        }

    def probe(self):
        self.reset_timer()
        sensor_val = self.usrp_block.get_mboard_sensor(self.sensor)
        label, sentence = sensor_val.to_pp_string().split(': ', 1)
        parsed = self.parse_nmea_sentence(sentence)
        if parsed['valid'] or not self.require_valid:
            msg = pmt.to_pmt({
                'core:latitude': parsed['latitude'],
                'core:longitude': parsed['longitude'],
            })
            self.message_port_pub(pmt.intern('out'), msg)
        else:
            print("WARNING: Not emitting GPS message, fix invalid.")
