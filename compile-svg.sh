#!/usr/bin/env bash

sed 's/id="defs1" \/>/id="defs1">/' mask.svg | sed 's/<\/svg>/<\/defs><\/svg>/' > mask2.svg
npx fctx-compiler mask2.svg
rm mask2.svg
