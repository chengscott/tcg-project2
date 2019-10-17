.PHONY: run judge format check clean
all: threes

threes:
	g++ -std=c++11 -O3 -o threes threes.cpp

run: threes
	./threes --save=stat.txt

judge: run
	/tcg/files/pj-1-judge --judge=stat.txt

format:
	clang-format -i *.cpp *.h

check:
	clang-tidy threes.cpp -checks=bugprone-*,clang-analyzer-*,modernize-*,performance-*,readability-* -- -std=c++11

clean:
	rm -rf threes stat.txt weights.bin action agent board episode statistic pattern *.dSYM