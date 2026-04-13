#!/bin/bash
 
#SBATCH --job-name=SorterHunter
#SBATCH -N 1
#SBATCH -c 128
#SBATCH --mem=100G
#SBATCH --time=0-00:10:00
#SBATCH --gres=tmp:1G
#SBATCH -p test
#SBATCH -o runs/out-%j.log
#SBATCH -e runs/err-%j.log
 
# Commands to be run:
./build/src/SorterHunter "config.txt"