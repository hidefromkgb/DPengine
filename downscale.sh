#!/usr/bin/env bash



# Enum(input, output, pict, printpath)
function Enum {
  art=$(echo "$3" | sed 's/[Gg][Ii][Ff]$/art/');
  dif=1;     # no file
  if [ -f "$2/$3" ]; then
    gifsicle --scale 2.0 --optimize < "$2/$3" > "$2/$3[tmp]";
    if [ -z "$(gifdiff -q "$1/$3" "$2/$3[tmp]")" ]; then
      dif=0; # same file
    else
      dif=2; # different file
    fi;
    rm "$2/$3[tmp]";
  fi;
  if ((dif)); then
    if ((dif>1)); then
      cp "$2/$3" "$2/$3[prev]";
    fi;
    gifsicle --scale 0.5 --optimize < "$1/$3" > "$2/$3[tmp]";
    gifsicle --scale 2.0 --optimize < "$2/$3[tmp]" > "$2/$3";
    if [ -n "$(gifdiff -q "$1/$3" "$2/$3")" ]; then
      gifsicle --optimize < "$1/$3" > "$2/$3";
      rm "$2/$3[tmp]";
      echo -n "[!!!";
    else
      mv "$2/$3[tmp]" "$2/$3";
      echo -n "[   ";
    fi;
  fi;
  if $(find "$1" | grep -qi "$art"); then
    iart="$(find "$1" | grep -i "$art")";
    cp "$iart" "$2/$art";
    if ((dif)); then
      echo -n "A] ";
    fi;
  else
    if ((dif)); then
      echo -n " ] ";
    fi;
  fi;
  if ((dif)); then
    echo "$4";
  fi;
}



ipath="Ponies";
opath="anim";



folders=$(find "./$ipath" -mindepth 1 -maxdepth 1 | sort | uniq);
mkdir "$opath";

IFST=$IFS;
IFS=$(echo -ne "\n\b");
erasepath="sed 's/\(.*\)\/\([^\/]*\)$/\2/'";

for input in $folders; do
  output="$(echo "$input" | sed "s/\.\/$ipath/$opath/")";
  if $(echo "$output" | grep -qv '\.'); then
    echo -e "\n$input";
    mkdir "$output";
    effect="$(cat "$input/pony.ini" | grep Effect | cut -d, -f4)";
    behavior="$(cat "$input/pony.ini" | grep Behavior | cut -d, -f7)";

    files=$(echo -e "$behavior\n$effect" | sed 's/\"//g' | grep -i '\.gif' | sort | uniq);
    for pict in $files; do
      pict="$(find "$input" | grep -i "\/$pict" | eval $erasepath)";
      Enum "$input" "$output" "$pict" "$input/$pict";
    done;

    mkdir "$output/~";
    files=$(echo -e "$behavior\n$effect" | sed 's/\"//g' | grep -i '\.png' | sort | uniq);
    for png in $files; do
      png="$(find "$input" | grep -i "\/$png" | eval $erasepath)";
      gif=$(echo "$png" | sed 's/[Pp][Nn][Gg]$/gif/');
      convert "$input/$png" "$output/~/$gif";
      Enum "$output/~" "$output" "$gif" "$input/$png";
      rm "$output/~/$gif";
    done;
    rmdir "$output/~";

    files=$(find "$input" | grep -vi "\.png" | grep -vi "\.gif" | grep -vi "\.art" | eval $erasepath | grep '\.');
    for misc in $files; do
      cp "$input/$misc" "$output/$misc";
      echo "       $input/$misc";
    done;
  fi;
done;

IFS=$IFST;
