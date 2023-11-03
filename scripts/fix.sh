#!/bin/bash

for fl in $(find -type f -name "*.[c,h]");
do
	VERSION_CONTROL="none" indent -linux -l120 -i4 -nut $fl
done
