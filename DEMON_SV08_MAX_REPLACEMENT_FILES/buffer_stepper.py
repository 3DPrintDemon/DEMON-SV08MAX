# Copyright (C) 2025  Sovol3d <info@sovol3d.com>
#
# This file may be distributed under the terms of the GNU GPLv3 license.

import stepper, chelper, logging, toolhead
from . import force_move

MIN_KIN_TIME = 0.050
SDS_CHECK_TIME = 0.001 

class BufferStepper:
    def __init__(self, config):
        self.printer = config.get_printer()
        stepper_name = config.get_name().split()[1]
        self.gcode = self.printer.lookup_object('gcode')
        for n, mcu in self.printer.lookup_objects(module='mcu'):
            if mcu.get_name() == 'buffer_mcu':
                self.mcu = mcu
        if self.mcu is None:
            raise self.printer.config_error("Must set separate MCU for buffer_stepper")
        self.debug = config.get('debug', False)
        self.reactor = self.printer.get_reactor()
        self.kin_flush_delay = SDS_CHECK_TIME
        self.last_kin_flush_time = 0.
        self.buffer_time_start = config.getfloat('buffer_time_start', 0.050, above=0.)

        buttons = self.printer.load_object(config, 'buttons')
        self.push = config.get('push_pin')
        buttons.register_buttons([self.push], self._push_handler)
        self.push_triggered = False
        self.min_event_systime = self.reactor.NEVER
        self.rail = stepper.PrinterStepper(config)
        self.event_delay = config.getfloat('event_delay', 2.0, above=0.)
        self.current_check_id = 0

        self.steppers = [self.rail]
        self.velocity = config.getfloat('velocity', 150., above=0.)
        self.accel = self.homing_accel = config.getfloat('accel', 5000., minval=0.)
        self.push_length = config.getfloat('push_length', 25., minval=1.)
        self.next_cmd_time = 0.
        ffi_main, ffi_lib = chelper.get_ffi()
        self.trapq = ffi_main.gc(ffi_lib.trapq_alloc(), ffi_lib.trapq_free)
        self.trapq_append = ffi_lib.trapq_append
        self.trapq_finalize_moves = ffi_lib.trapq_finalize_moves
        self.stepper_kinematics = ffi_main.gc(ffi_lib.cartesian_stepper_alloc(b'x'), ffi_lib.free)
        self.rail.set_stepper_kinematics(self.stepper_kinematics)
        self.rail.set_trapq(self.trapq)

        # Register commands
        self.printer.register_event_handler("klippy:ready", self._handle_ready)
        self.print_stats = self.printer.load_object(config, 'print_stats')
        self.gcode.register_mux_command('BUFFER_STEPPER', "STEPPER",
                                    stepper_name, self.cmd_BUFFER_STEPPER,
                                    desc=self.cmd_BUFFER_STEPPER_help)

    def _push_handler(self, eventtime, state):
        if state == self.push_triggered:
            return
        self.push_triggered = state
        eventtime = self.reactor.monotonic()
        if eventtime < self.min_event_systime:
            return
        self.current_check_id += 1 
        if self.push_triggered:
            global_var = self.printer.lookup_object('gcode_macro _global_var')
            status = global_var.get_status(eventtime)
            is_push_buffer = status.get('is_push_buffer', True)
            if is_push_buffer:
                self.do_move(self.push_length, self.velocity, self.accel)       
                if self.print_stats.state == 'printing':
                    self.gcode.run_script_from_command("NOZZLE_CLOG_CHECK")
                initial_push_state = self.push_triggered
                check_id = self.current_check_id 
                self.reactor.register_async_callback(
                    lambda evt: self._check_filament_jam(evt, initial_push_state, check_id),
                    eventtime + self.get_move_duration(self.push_length, self.velocity, self.accel) + self.event_delay
                )
    def _handle_ready(self):
        self.min_event_systime = self.reactor.monotonic() + 0.2
    def sync_print_time(self):
        curtime = self.reactor.monotonic()
        est_print_time = self.mcu.estimated_print_time(curtime)
        kin_time = max(est_print_time + MIN_KIN_TIME, self.last_kin_flush_time)
        kin_time += self.kin_flush_delay
        min_print_time = max(est_print_time + self.buffer_time_start, kin_time)
        if min_print_time > self.next_cmd_time:
            self.next_cmd_time = min_print_time
    def do_enable(self, enable):
        self.sync_print_time()
        stepper_enable = self.printer.lookup_object('stepper_enable')
        if enable:
            for s in self.steppers:
                se = stepper_enable.lookup_enable(s.get_name())
                se.motor_enable(self.next_cmd_time)
        else:
            for s in self.steppers:
                se = stepper_enable.lookup_enable(s.get_name())
                se.motor_disable(self.next_cmd_time)
        self.sync_print_time()
    def do_set_position(self, setpos):
        self.rail.set_position([setpos, 0., 0.])
    def do_move(self, movepos, speed, accel, sync=True):
        self.sync_print_time()
        self.rail.set_position((0., 0., 0.))
        dist = movepos
        axis_r, accel_t, cruise_t, cruise_v = force_move.calc_move_time(
            dist, speed, accel)
        self.trapq_append(self.trapq, self.next_cmd_time, accel_t, cruise_t, accel_t,
                          0., 0., 0., axis_r, 0., 0., 0., cruise_v, accel)
        self.next_cmd_time = self.next_cmd_time + accel_t + cruise_t + accel_t
        self.rail.generate_steps(self.next_cmd_time)
        self.trapq_finalize_moves(self.trapq, self.next_cmd_time + 99999.9, self.next_cmd_time + 99999.9 )
        self.mcu.flush_moves(self.next_cmd_time, self.next_cmd_time - 30.)
    def get_move_duration(self, movepos, speed, accel):
        dist = movepos
        axis_r, accel_t, cruise_t, cruise_v = force_move.calc_move_time(
            dist, speed, accel)
        return accel_t + cruise_t + accel_t
    def _check_filament_jam(self, eventtime, initial_push_state, check_id):
        if self.current_check_id != check_id or self.push_triggered != initial_push_state or self.print_stats.state == 'paused' or getattr(self, '_in_pause', False):
            return
        filament_sensor = self.printer.lookup_object('filament_switch_sensor filament_sensor')
        filament_detected = filament_sensor.runout_helper.filament_present
        if filament_detected:
            if self.push_triggered:
                self.gcode.respond_info("Filament jam detected! Push pin is still triggered after move.")
                if self.print_stats is not None and self.print_stats.state == 'printing':
                    try:
                        self._in_pause = True
                        self.gcode.run_script_from_command("SET_PIN PIN=LED_Green VALUE=0.00")
                        self.gcode.run_script_from_command("SET_PIN PIN=LED_Blue VALUE=1.00")
                        self.gcode.run_script_from_command("PAUSE")
                        self.gcode.run_script_from_command("SET_GCODE_VARIABLE MACRO=variables VARIABLE=winding_status VALUE=True")
                        self.gcode.respond_info("Filament winding")
                    finally:
                        self._in_pause = False
            else:
                self.gcode.run_script_from_command("SET_GCODE_VARIABLE MACRO=variables VARIABLE=winding_status VALUE=False")
    cmd_BUFFER_STEPPER_help = "Command a manually configured stepper"
    def cmd_BUFFER_STEPPER(self, gcmd):
        enable = gcmd.get_int('ENABLE', None)
        if enable is not None:
            self.do_enable(enable)
        setpos = gcmd.get_float('SET_POSITION', None)
        if setpos is not None:
            self.do_set_position(setpos)
        speed = gcmd.get_float('SPEED', self.velocity, above=0.)
        accel = gcmd.get_float('ACCEL', self.accel, minval=0.)
        if gcmd.get_float('MOVE', None) is not None:
            movepos = gcmd.get_float('MOVE')
            sync = gcmd.get_int('SYNC', 1)
            self.do_move(movepos, speed, accel, sync)
    def get_position(self):
        return [self.rail.get_commanded_position(), 0., 0., 0.]
    def set_position(self, newpos, homing_axes=()):
        self.do_set_position(newpos[0])
    def get_last_move_time(self):
        self._calc_print_time()
        return self.next_cmd_time
    def dwell(self, delay):
        self.next_cmd_time += max(0., delay)
    def drip_move(self, newpos, speed, drip_completion):
        self.do_move(newpos[0], speed, self.homing_accel)
    def get_kinematics(self):
        return self
    def get_steppers(self):
        return self.steppers
    def calc_position(self, stepper_positions):
        return [stepper_positions[self.rail.get_name()], 0., 0.]
    def debug_logging(self, message):
        self.gcode.respond_info(message)
    def get_status(self, eventtime):
        return {
            'push_triggered': self.push_triggered,
    }

def load_config_prefix(config):
    return BufferStepper(config)