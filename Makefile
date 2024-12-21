.PHONY: all clean test_uuid

CXX := clang++
CXXFLAGS := -std=gnu++23 -fsized-deallocation -Wall -Wextra -Wpedantic
LDFLAGS := -flto=thin -fuse-ld=lld -lbacktrace -lstdc++exp
SANITIZER := -fsanitize=address,undefined
DEBUGFLAGS := -Og -fno-omit-frame-pointer -gdwarf-5 -glldb
RELEASEFLAGS := -O3 -march=native -DNDEBUG
OPTIMIZATION := $(DEBUGFLAGS)

all: main test_uuid

main: main.out main.ll

main.ll: main.cxx
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -S -emit-llvm -o $@ $<

main.out: main.cxx
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) $(LDFLAGS) -o $@ $<

test_uuid: test_uuid.out test_uuid.ll

test_uuid.out: main.cxx
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) $(LDFLAGS) -luuid -DTEST_UUID -o $@ $<

test_uuid.ll: main.cxx
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -DTEST_UUID -S -emit-llvm -o $@ $<

clean:
	rm -f *.out *.ll
