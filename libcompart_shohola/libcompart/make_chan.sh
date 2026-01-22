#!/bin/sh
# Part of the "chopchop" project at the University of Pennsylvania, USA.
# Author: Nik Sultana, 2019.

check_exists() {
FILE=$1
if [ -e "${FILE}" ]
then
  echo "${FILE} already exists" >&2
  exit 1
fi
}

if [ -z $1 ]
then
  echo "Need to provide base name" >&2
  exit 1
fi

base_name=$1

CHAN1="${base_name}_chan1"
CHAN2="${base_name}_chan2"
CHAN_2m_r=${base_name}_2m_r
CHAN_m2_w=${base_name}_m2_w
CHAN_m2_r=${base_name}_m2_r
CHAN_2m_w=${base_name}_2m_w

check_exists ${CHAN1}
check_exists ${CHAN2}
check_exists ${CHAN_2m_r}
check_exists ${CHAN_m2_w}
check_exists ${CHAN_m2_r}
check_exists ${CHAN_2m_w}

mkfifo ${CHAN1}
mkfifo ${CHAN2}
ln -s ${CHAN1} ${CHAN_2m_r}
ln -s ${CHAN1} ${CHAN_m2_w}
ln -s ${CHAN2} ${CHAN_m2_r}
ln -s ${CHAN2} ${CHAN_2m_w}
