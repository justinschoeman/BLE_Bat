#!/usr/bin/python3

# pip3 install influxdb-client
# influx config create --config-name battery --host-url http://localhost:8086 --org Home --token  1xqNDMN9LyXbHh9HwdmUBbghIKi1DuKtjR0C7er0Em630zR0x_NwjHRhym3oUqzV9EumjEayfBHvGkUS9IFllQ== --active

import re
import serial
from time import sleep
from datetime import datetime
import traceback
import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS

# config
# general
cfg_echo = True
# serial
cfg_port = '/dev/ttyUSB0'
cfg_baud = 115200
#influx
cfg_bucket = "battery"
cfg_org = "Home"
cfg_token = "1xqNDMN9LyXbHh9HwdmUBbghIKi1DuKtjR0C7er0Em630zR0x_NwjHRhym3oUqzV9EumjEayfBHvGkUS9IFllQ=="
cfg_url="http://localhost:8086/"


def parse(wa, ln):
    #print('log: %s' % ln)
    m = re.match('^([^ _]+)_([^ ]+) = ([-+]{0,1}[0-9]+[.]{0,1}[0-9]*)$', ln)
    if m is None:
        print('INVALID LOG LINE (%s)' % ln)
        return
    print("INFLUX LOG (%s - %s) = (%s)" % (m.group(1), m.group(2), m.group(3)))
    p = influxdb_client.Point(m.group(1)).field(m.group(2), float(m.group(3)))
    wa.write(bucket = cfg_bucket, org = cfg_org, record = p)


def log():
    try:
        # open Influx
        influx = influxdb_client.InfluxDBClient(
            url = cfg_url,
            token = cfg_token,
            org = cfg_org,
            debug = False
        )
        #write_api = influx.write_api(write_options=SYNCHRONOUS)
        write_api = influx.write_api(write_options=influxdb_client.WriteOptions(batch_size=50, flush_interval=1000, jitter_interval=200))


        port = serial.Serial(port = cfg_port, baudrate = cfg_baud, timeout = 2)
        #port = serial.serial_for_url(cfg_port, baudrate = cfg_baud, timeout = 2, rtscts = False, dsrdtr = False, do_not_open=True) 
        #port.dtr = 0
        #port.rts = 0
        #port.open()

        print("Opened serial: '%s'" % port.name)
        errcnt = 0
        slog = False
        while True:
            ln = port.readline().decode('ascii', 'replace').strip()
            #print("read: '%s'" % ln)
            if cfg_echo:
                print('%s : %s' % (datetime.now(), ln))
            if ln == '':
                print("timeout")
                errcnt = errcnt + 1
                if errcnt > 10:
                    print('NO DATA FOR 20s? TRY RESTART!');
                    # https://forum.arduino.cc/t/pyserial-not-resetting-program/39077/3
                    port.setDTR(False) # Drop DTR
                    sleep(0.022)       # Read somewhere that 22ms is what the UI does.
                    port.setDTR(True)  # UP the DTR back
                    errcnt = 0
            else:
                errcnt = 0
            if slog:
                if ln == '>>>>>':
                    print('Log end...')
                    slog = False
                else:
                    parse(write_api, ln)
            else:
                if ln == '<<<<<':
                    print('Log start...')
                    slog = True
    except BaseException as error:
        print('LOG exception occurred: {}'.format(error))
        print(traceback.format_exc())
        raise(error)


log()
