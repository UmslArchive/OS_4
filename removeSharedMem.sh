#!/bin/bash

pkill "usrPs"

ipcrm -M 0x66666666
ipcrm -M 0x77777777
ipcrm -M 0x88888888
ipcrm -M 0x99999999
