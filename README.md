# smartlynq_static_ip

This is a Linux utility for programming a static IP address into a Xilinx SmartLynq JTAG programmer.  To use:

(1) Have either Vivado or Vivado Lab Edition installed on your computer.

(2) Download this repository.  

(3) Plug in a SmartLynq JTAG programmer via USB.  Make *absolutely certain* that only one JTAG programmer is plugged into your computer!  (Failure to ensure that only one JTAG programmer is plugged in is likely to brick a SmartLynq!).  There should be no external power supply or ethernet cable plugged into the SmartLynq, just the USB cable.

(4) The SmartLynq should show an IP address on the top line of the screen.   If it doesn't, unplug the USB cable, and plug it back in.  It may take several tries before the SmartLynq shows an IP address.  

(5) The IP address at the top of the SmartLynq is the IP address of the IP-over-USB interface.  In the instructions below, it is refered to as the USB_IP.  The ethernet static IP adress you wish to program into the SmartLynq will be called STATIC_IP.  

(6) Examine the "vivado" setting in file "smartlynq_static_ip.conf".  Make sure that it points to where Vivado (or Vivado Lab Edition) is installed on your computer.  

(7) Run the command:
~~~
./smartlynq_static_ip <USB_IP> <STATIC_IP>
~~~

For instance, if the SmartLynq display shows a USB_IP of 10.0.0.2 and you want to set a static IP address of 10.11.12.3, run the command:
~~~
./smartlynq_static_ip 10.0.0.2 10.11.12.3
~~~

(8) That it's, you're done!
