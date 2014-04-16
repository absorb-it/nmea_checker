nmea_checker
============

tool to verify checksum of NMEA-Data, can read from file/serial and write to file/serial

# ./nmea_checker 
NMEA Checker, version 0.2
------------------------------------------------------------------------
NMEA Checker reads Input from one serial line or file and verifies
 the NMEA checksum. If ok, NMEA sentence will be transferred to
 the Output device or file.
------------------------------------------------------------------------
usage: nmea_checker [-d] [-t] InputFile|InputDevice [OutputFile|OutputDevice]
       device can also be '/dev/ptmx' pseudoterminal
-d      log all the data to nmea_checker_[all|ok|wrong].log
-t      include timestamp in log-data (but not in output file)

example: ./nmea_checker -d -t /dev/ttyUSB0 /dev/ttyUSB1


