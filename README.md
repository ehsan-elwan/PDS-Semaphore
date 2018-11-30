# PDS-Semaphore
Basic code and implementation of semaphores in C language, producer consumer scenario with circular buffer.

To compile in terminal use:
gcc -pthread inFile.c -o outputFile

Since the Mac OS doesn't allow unnamed semaphores, there is two versions
Linux: using unnamed semaphores
Mac OS: using named semaphores
PS: for Mac version please run clean_NamedSems to unlink the semaphores in case of the process was interrupted

Usage:
Version 1 and 2 need 3 parameters: nbOfProducers, nbOfConsumers, SizeOfBuffer.
Version 3 need 4: nbOfProducers, nbOfConsumers, nbOfTypes, SizeOfBuffer.
