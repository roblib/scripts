#!/bin/bash

# Mac-compatible script
# Writes the contents of an unprotected DVD to an MPEG-2 file in the current directory.

# Temporarily set the for-loop delimiter to deal with filenames with spaces.
# Source: http://www.cyberciti.biz/tips/handling-filenames-with-spaces-in-bash.html
SAVEIFS=$IFS
IFS=$(echo -en "\n\b")

# Iterate over /Volumes/*/VIDEO_TS, i.e. find all volumes that are DVDs
for VTSPATH in $(find /Volumes -name VIDEO_TS -maxdepth 2 -mindepth 2)
do
  # Get disk name
  DVDNAME=${VTSPATH#/Volumes/}
  DVDNAME=${DVDNAME%/VIDEO_TS}
  # Find .VOB files
  DVDPATH=$(find "$VTSPATH" -name "VTS*.VOB" 2>/dev/null | tr "\\n" "|" )
  # Exit if files not found
  if [ ! "$DVDPATH" ]; then
    echo ".VOB files not found in $VTSPATH. Exiting."
    exit
  fi 
  
  # Get user input on the output filename; provide an option to use the Volume name
  echo -e "\n\n$DVDNAME\n"
  read -p "That is the disk name. Is this the desired output filename? [y/N]: " -n 1 -r
  if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "\n\nEnter the desired output filename:"
    read inputname
    while [ $inputname = "" ]; do
      read inputname
    done
    inputname=$(echo $inputname | sed -e 's/ //g')
    echo $inputname
    DVDNAME=$inputname
  fi 
    
  # Perform the encoding
  ffmpeg -i concat:"$DVDPATH" -f mpeg -c:v copy -c:a mp2 $DVDNAME.mpg
  # Uncomment the following to give a system alert when the file is done. 
  #  osascript -e 'tell app "System Events" to display dialog "Your disk is done!"'
  # Eject the disk.
  drutil eject external
done
# Reset the for-loop delimiter
IFS=$SAVEIFS
exit
