#!/bin/bash 
# Used to generate html stats report from bittorrent logs

if [ $# -ne 2 ];
  then
    echo "Usage: ./bittorent_generate_stats_report.sh <bittorrent_transfer_folder> <output_folder>"
    echo "Ex: ./bittorent_generate_stats_report.sh pmem1 session_stats_report"
    exit 1
fi

INPUT_FOLDER=$1
OUTPUT_FOLDER=$2

LOGS=$(find $INPUT_FOLDER -name '*.log' -print | tr '\n' ' ')

if [ $LOGS -eq "" ];
  then
  echo "No log file found"
  exit 1
fi

python3 ./util/parse_session_stats.py $LOGS

mv session_stats_report/ ./$OUTPUT_FOLDER