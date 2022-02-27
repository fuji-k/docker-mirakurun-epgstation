#!/bin/bash

/usr/bin/mono /usr/local/Amatsukaze/exe_files/AmatsukazeAddTask.exe -r "C:\\APL\\Movie\\Amatsukaze" -f "$INPUT" -ip "192.168.0.2" -p 32768 -o "\\\\RPI4\\Share\\docker\\dmer\\recorded" --remote-dir "\\\\RPI4\\Share\\docker\\dmer\\recorded" -s "QSVEnc-720@30p" --priority 3 --no-move

sleep 10

while true
do
    if [ -d "/usr/local/EPGStation/recorded/succeeded" ] ; then
        sleep 10
    else
        break
    fi
done
