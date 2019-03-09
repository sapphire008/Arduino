# -*- coding: utf-8 -*-
"""
Created on Fri Jun 05 00:57:19 2015

@author: Edward
"""

import sys, glob, serial

def listSerialPorts():
    """List serial ports
    : raises EnvironmentError: On unsupported or unknown OS platforms
    : returns: A list of available serial ports
    """    
    if sys.platform.startswith('win'):
        import win32com.client
        wmi = win32com.client.GetObject ("winmgmts:")
        ports = dict()
        for com in wmi.InstancesOf("Win32_SerialPort"):
            ports[str(com.Name)] = str(com.DeviceID)
    elif sys.platform.startswith('linux') or sys.platform.startwith('cygwin'):
        # this is to eclude your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-a]*')
    elif  sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported OS platform')        
    
    return ports
    
def listUSBPorts(): # Not tested
    """List USB ports
    : raises EnvironmentError: On unsupported or unknown OS platforms
    : return: A list of available USB port
    """
    if sys.platform.startswith('win'):
        import win32com.client    
        wmi = win32com.client.GetObject ("winmgmts:")
        ports = dict()
        for usb in wmi.InstancesOf("Win32_USBHub"):
            ports[str(usb.Name)] = str(usb.DeviceID)
    elif sys.platform.startswith('linux') or sys.platform.startwith('cygwin'):
        # this is to eclude your current terminal "/dev/tty"
        ports = glob.glob('/dev/ttyusb[A-Za-a]*')
    elif  sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.usb*')
    else:
        raise EnvironmentError('Unsupported OS platform')
    
    return ports
    

# Read and write Arduino serial port
s = serial.Serial('COM3', baudrate=9600, timeout=5, writeTimeout=5,interCharTimeout=5)
s.read() # read, but will time out
s.write('0') # write, but will time out
s.close() # close the port
s.isOpen() # check if the port is still open
    
    






