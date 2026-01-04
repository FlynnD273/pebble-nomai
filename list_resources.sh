#!/usr/bin/env bash

for file in resources/*.fpath; do
	name="$(basename "$file")"
	cat << EOF
{
	"file": "$name",
	"name": "MASK_${name%.*}",
	"type": "raw"
},
EOF
done
cat << EOF
{
	"file": "OUTERwilds-Normal.ffont",
	"name": "WildsFont",
	"type": "raw"
},
{
	"file": "icon.png",
	"name": "Icon",
	"type": "bitmap",
	"menuIcon": "true"
}
EOF
