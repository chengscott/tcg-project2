.PHONY: run judge format check clean
.PHONY: run, judge, format, check, clean
all: threes

threes: *.cpp *.h
	g++ -std=c++11 -march=native -O3 -o threes threes.cpp

run: threes
	./threes --play='load=weights.bin alpha=0' --save=stat.txt

judge: run
	/tcg/files/pj-2-judge --judge=stat.txt

format:
	clang-format -i *.cpp *.h

check:
	clang-tidy threes.cpp -checks=bugprone-*,clang-analyzer-*,modernize-*,performance-*,readability-* -- -std=c++11

clean:
	rm -rf threes stat.txt action agent board episode pattern statistic *.dSYM