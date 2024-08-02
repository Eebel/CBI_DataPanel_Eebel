# CBI_DataPanel_Eebel
R2-D2 Data Panel and Charge Bay light control.  Allows a PWM signal to enable/disable the lights.  The original sketch was written by CuriousMarc and modified by VAShadow to add the CBI lights.
This code will run on any Arduino Nano/Uno and the like.  It was designed to operate the lighting boards available on Astromech.net.

Added the ability to disable and clear the lighting sequences when each door is closed by reading two separate PWM signals.  One signal for each lighting module (CBI and DataPanel
- Renamed to CBI_DataPanel 2.1 - Eebel
- I know tis code is not optimized.  However, it works.
- IMPORTANT!!!! Both signal pins need to be powered and sending the correct PWM signal (above or below 1500 micorseconds) for the code to work
  - If only one pin is powered the sequence lags due to the pulseIn() command attempting to read an invalid data
  - Both pins can safely be depowered when not in use (i.e. the doors are closed)
  - Updated the Battery Level meters to include a scale value to tune the lights to your battery voltage.
