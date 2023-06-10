default:
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -g -c util.c -o util.o
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -g -c gmp256k1/Int.cpp -o Int.o -lgmp
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -g -c gmp256k1/GMP256K1.cpp -o SECP256K1.o -lgmp
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -g -c gmp256k1/IntMod.cpp -o IntMod.o -lgmp
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -g -flto -c gmp256k1/Random.cpp -o Random.o -lgmp
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -flto -c gmp256k1/IntGroup.cpp -o IntGroup.o -lgmp
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -g -c SaveProgress.cpp -o SaveProgress.o -lgmp
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -g -o ReadFileSaveProgress ReadFileSaveProgress.cpp  util.o Int.o IntMod.o  Random.o IntGroup.o SaveProgress.o -lm -lpthread -lgmp
	g++ -m64 -march=native -mtune=native -mssse3 -Wno-unused-result -Wno-write-strings -Ofast -ftree-vectorize -g -o testSaveProgress testSaveProgress.cpp  util.o Int.o IntMod.o  Random.o IntGroup.o SaveProgress.o -lm -lpthread -lgmp
	rm -r *.o
