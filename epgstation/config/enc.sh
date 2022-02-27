#!/bin/bash

DIRNAME=${INPUT%/*}
FANAME_EXT="${INPUT##*/}"
FNAME="${FANAME_EXT%.*}"

cd $DIRNAME
cp "$FANAME_EXT" temp.ts

ARIBTS2ASS="/usr/local/bin/arib-ts2ass"
JLSE="/usr/local/bin/jlse"
CUTASS="/usr/local/bin/cutass"
FFMPEG="/usr/local/bin/ffmpeg"
AVSBASEPATH="/root/JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial/result/"
$ARIBTS2ASS -o before.ass temp.ts
export HOME="/root"
$JLSE -i temp.ts -f true -e true -t cutcm_logo -o " -c:v h264_omx -vf yadif,scale=-2:720 -b:v 4M -aspect 16:9 -r 30000/1001 -c:a aac -ab 192K -ac 2 -bsf:a aac_adtstoasc"
cp $AVSBASEPATH/temp/in_cutcm.avs .
if [ -e "before.ass" ];then
    $CUTASS -o "temp.ass" -t in_cutcm.avs "before.ass"
    $FFMPEG -i "temp.mp4" -i "temp.ass" -c:v copy -c:a copy temp.mkv
else
    $FFMPEG -i "temp.mp4" -c:v copy -c:a copy temp.mkv
fi
mv temp.ts "$FANAME_EXT"
mv temp.mkv "$FNAME.mkv"
rm -f "before.ass"
rm -f "temp.mp4"
rm -f "temp.ass"
rm -f "in_cutcm.avs"
rm -f "temp.ts.lwi"
