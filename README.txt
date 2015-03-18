 /**************************************************************************/
 /*     Volume Rendering for Teddy Bear Stack of Raster Images             */ 
 /**************************************************************************/
 # Command line : ./TeddyVolViz.exe <PATH_TO_LST_FILE>
 # If input .lst file is not passed as a first argument, then the program will take "teddybear.lst" as default file name, 
 # expecting a valid file of that name exists in the same directory as the executable.
 # 
 # Press N: to switch to next rendering mode. If current rendering mode is Ortho slices, this will switch to Volume rendering.
 # Press P: to switch to previous rendering mode. Pressing keys N and P behaves as a toggle between Ortho Slice and Volume rendering
 # Press R: to toggle between enabling and disabling volume rendering inside a Region of Interest(ROI).
 # Press A: to render both Ortho slices and Volume together. Press R at this state to enable ROI for volume rendering