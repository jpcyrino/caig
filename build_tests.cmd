@echo off
echo Build test_lexicon.exe
gcc -o test_lexicon src\cu32.c src\lexicon.c src\test_lexicon.c -g
echo Build test_minseg.exe
gcc -o test_minseg src\cu32.c src\lexicon.c src\minseg.c src\test_minseg.c -g
echo Build test_lexhnd.exe
gcc -o test_lexhnd src\cu32.c src\lexicon.c src\minseg.c src\lexhnd.c src\test_lexhnd.c -g
