#!/bin/sh
# Part of the "chopchop" project at the University of Pennsylvania, USA.
# Author: Nik Sultana, 2019.

INPUT=hello_compartment.c
OUTPUTS=( other_compartment.c third_compartment.c )
NAMES=( "other compartment" "third compartment" )

for IDX in $(seq 0 $(( ${#OUTPUTS[@]} - 1 )) )
do
  if [ -e ${OUTPUTS[$IDX]} ]
  then
    echo "File \"${OUTPUTS[$IDX]}\" already exists" >&2
    exit 1
  fi
  sed "s/^\( *\)compart_start.*$/\1compart_as(\"${NAMES[$IDX]}\");/g" ${INPUT} > ${OUTPUTS[$IDX]}
done

sed -i "s/^\( *\)compart_init(NO_COMPARTS, comparts, default_config);$/\1struct compart_config my_config = default_config;\n\1my_config.start_subs = 0;\n\n\1compart_init(NO_COMPARTS, comparts, my_config);\n/g" ${INPUT}
