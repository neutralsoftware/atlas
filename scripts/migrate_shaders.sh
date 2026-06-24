#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Usage: $0 <directory> <version>"
  exit 1
fi

DIR="$1"
VERSION="$2"

if [ ! -d "$DIR" ]; then
  echo "Error: '$DIR' is not a directory"
  exit 1
fi

for file in "$DIR"/*; do
  [ -f "$file" ] || continue
  
  { echo "#version ${VERSION} core"; tail -n +2 "$file"; } > "$file.tmp" && mv "$file.tmp" "$file"
  
  echo "Updated: $file → #version ${VERSION} core"
done
