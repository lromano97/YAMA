#/!/bin/sh
cd ../Master/src
echo "Compilando Master"
gcc master.c -o master -lcommons -lpthread -lm
cd ../../Compilacion
