Readme_231106smw.txt

This shared folder
G:\My Drive\HARP_UDP\code

contains software for running a UDP client in 
(a) Windows10 with a C executable (from Allen Nance)
(b) Windows10 with MATLAB 2022b
(c) Raspberry Pi 4 with python

where, 4ch HARP running @ 200 kHz per chan and 
firmware 3B04230307

Only chans 1 and 2 @ 200 kHz are streamed out in UDP packets.
HARP UDP server starts upon mission start (begin 'B' command) and stays running until mission stopped.
All UDP client software codes check each UDP packet timestamps, and if not correct
assumes a sync loss and restarts client to resync.

See email pdf from Nance for UDP data format,
along with MATLAB and python code.

Firmware 3B05 YYMMDD - may be done by beginning of December 2023.
Planned changes will be 
(1) remove HRMSD and use single uSD card on CPU board for mass storage
(2) 4 chan @ 100 kHz/chan so all 4 chans can be UDP'ed out.
This will yield two weeks of recording with 1 TB uSD
(3) Minimize SRAM via removing HRMEM card, write continuously to uSD
(4) if possible, include HRIMU board
This configuration will be 3 boards and a hacked 3x96pin backplane:
CPU, A/D, IMU + bkplane




