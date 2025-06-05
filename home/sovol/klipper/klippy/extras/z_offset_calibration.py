# Support for reading frequency samples from ldc1612
#
# Copyright (C) 2024-2025 Sovol3d <info@sovol3d.com>
#
# This file may be distributed under the terms of the GNU GPLv3 license.
from . import probe, probe_eddy_current, manual_probe
import math
import configparser

class ZoffsetCalibration:
    def __init__(self, config):
        self.printer = config.get_printer()
        self.config = config
        x_pos_endstop, y_pos_endstop = config.getfloatlist("endstop_xy_position", count=2)
        self.endstop_x_pos, self.endstop_y_pos = x_pos_endstop, y_pos_endstop
        self.z_hop = config.getfloat("z_hop", default=2.0)
        self.z_hop_speed = config.getfloat('z_hop_speed', 5., above=0.)
        self.zconfig = config.getsection('stepper_z')
        self.endstop_pin = self.zconfig.get('endstop_pin')
        self.speed = config.getfloat('speed', 180.0, above=0.)
        self.internal_endstop_offset = config.getfloat('internal_endstop_offset', default=0.)
        self.gcode = self.printer.lookup_object('gcode')
        self.gcode_move = self.printer.lookup_object('gcode_move')
        self.gcode.register_command("Z_OFFSET_CALIBRATION", self.cmd_Z_OFFSET_CALIBRATION, desc=self.cmd_Z_OFFSET_CALIBRATION_help)
        self.last_toolhead_pos = self.last_kinematics_pos = None
        self.non_contact_probe_name = config.get('non_contact_probe', None)
        self.contact_probe_name = config.get('contact_probe', None)
        pprobe = self.printer.lookup_object("probe")
        self.x_offset, self.y_offset, self.z_offset = pprobe.get_offsets()
    def _call_macro(self, macro):
        self.gcode.run_script_from_command(macro)
    def cmd_Z_OFFSET_CALIBRATION(self, gcmd):
        try:
            _non_contact_probe = self.printer.lookup_object(self.non_contact_probe_name)
            _contact_probe = self.printer.lookup_object(self.contact_probe_name)
        except:
            raise gcmd.error('ZoffsetCalibration: Failed to get object.')
        if hasattr(_non_contact_probe, 'run_non_contact_calibrate') is False:
            raise gcmd.error('ZoffsetCalibration: The [run_non_contact_calibrate] in the [non_contact_probe] object is not defined.')
        if hasattr(_contact_probe, 'run_contact_probe') is False:
            raise gcmd.error('ZoffsetCalibration: The [run_contact_probe] in the [contact_probe] object is not defined.')
        if _non_contact_probe.calibration.is_calibrated() == True and gcmd.get("METHOD", "default") == 'default':
            gcmd.respond_info("ZoffsetCalibration: Eddy data already exists")
            return
        if self.config.has_section('homing_override'):
            phoming = self.printer.lookup_object('homing_override')
        else:
            phoming = self.printer.lookup_object('homing')
        self.toolhead = self.printer.lookup_object('toolhead')
        pheater_bed = self.printer.lookup_object('heater_bed')
        pheater_extruder = self.printer.lookup_object('extruder')
        z_max_position = self.zconfig.getfloat('position_max')
        bed_temp = gcmd.get_float('BED_TEMP', default=65.0)
        extruder_temp = gcmd.get_float('EXTRUDER_TEMP', default=130.0)
        # Heat up
        pheaters = self.printer.lookup_object('heaters')
        ## set temp
        pheaters.set_temperature(pheater_bed.heater, bed_temp, wait=False)
        pheaters.set_temperature(pheater_extruder.heater, extruder_temp, wait=False)
        ## wait for heating
        pheaters.set_temperature(pheater_bed.heater, bed_temp, wait=True)
        pheaters.set_temperature(pheater_extruder.heater, extruder_temp, wait=True)
        # Home xy
        curtime = self.printer.get_reactor().monotonic()
        if 'xy' not in self.toolhead.get_status(curtime)['homed_axes']:
            gcmd_G28 = self.gcode.create_gcode_command("G28", "G28", {'X': 0, 'Y': 0})
            phoming.cmd_G28(gcmd_G28)
        pos = self.toolhead.get_position()
        z_limit_position = z_max_position + 15
        pos[2] = z_limit_position
        self.toolhead.set_position(pos, homing_axes=(0, 1, 2))
        self.set_z_offset(offset=0.)
        # Move to probe position
        gcmd.respond_info("ZoffsetCalibration: Toolhead move ...")
        self.toolhead.manual_move([self.endstop_x_pos, self.endstop_y_pos], self.speed)
        # Contact probe calibration
        gcmd.respond_info("ZoffsetCalibration: Toolhead probing ...")
        zendstop_p = _contact_probe.run_contact_probe(gcmd)
        pos = self.toolhead.get_position()
        pos[2] = z_max_position - self.z_hop
        self.toolhead.set_position(pos, homing_axes=(0, 1, 2))
        reprobe_cnt = 1
        while True:
            if(reprobe_cnt >= 10):
                self.gcode.run_script_from_command('M117 Tip code: 109')
                raise gcmd.error('ZoffsetCalibration: Toolhead probe more than ten times.')
            ## perform z hop
            if self.z_hop:
                pos[2] = self.toolhead.get_position()[2] + self.z_hop
                if pos[2] > z_max_position:
                    pos[2] = z_max_position
                self.toolhead.manual_move([None, None, pos[2]], 5)
            gcmd.respond_info("ZoffsetCalibration: Toolhead verifying the difference between before and after %d/10." % (reprobe_cnt))
            zendstop_p1 = _contact_probe.run_contact_probe(gcmd)
            diff_z = abs(zendstop_p1[2] - zendstop_p[2])
            zendstop_p = zendstop_p1
            if diff_z <= 0.0125:
                gcmd.respond_info("ZoffsetCalibration: Toolhead check success.")
                break
            reprobe_cnt += 1
        # Non-contact probe calibration
        _non_contact_probe.run_non_contact_calibrate(gcmd, self.internal_endstop_offset, self.z_hop_speed)
    def set_z_offset(self, offset):
        # reset pssible existing offset to zero
        gcmd_offset = self.gcode.create_gcode_command("SET_GCODE_OFFSET",
                                                      "SET_GCODE_OFFSET",
                                                      {'Z': 0})
        self.gcode_move.cmd_SET_GCODE_OFFSET(gcmd_offset)
        # set new offset
        gcmd_offset = self.gcode.create_gcode_command("SET_GCODE_OFFSET",
                                                      "SET_GCODE_OFFSET",
                                                      {'Z': offset})
        self.gcode_move.cmd_SET_GCODE_OFFSET(gcmd_offset)
    cmd_Z_OFFSET_CALIBRATION_help = "Test endstop and bed surface to calcualte g-code offset for Z"
    
def load_config(config):
    return ZoffsetCalibration(config)
