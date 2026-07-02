#!/bin/bash
scp -i ~/.ssh/AAC05.key -r Practica1_memoria_compartida vmuser@10.100.139.247:~
scp -i ~/.ssh/AAC05.key Zips/300k.csv vmuser@10.100.139.247:~/Practica1_memoria_compartida/

