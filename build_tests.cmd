@echo off
echo Build test_lexicon.exe
gcc -o test_lexicon src\cu32.c src\lexicon.c src\test_lexicon.c
echo Build test_minseg.exe
gcc -o test_minseg src\cu32.c src\lexicon.c src\minseg.c src\test_minseg.c
