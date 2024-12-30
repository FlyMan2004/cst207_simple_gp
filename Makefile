.PHONY: all clean

CXX := clang++
CXXFLAGS := -pipe -std=gnu++23 -fsized-deallocation -Wall -Wextra -fprebuilt-module-path=. -flto=thin
# On Linux
LDFLAGS := -fuse-ld=lld -lstdc++exp -luuid
SANITIZER := -fsanitize=address,undefined
# On Windows with msys2 ucrt64 (Broken binary. DO NOT USE)
# LDFLAGS := -fuse-ld=lld -lstdc++exp -lrpcrt4
# SANITIZER :=
DEBUGFLAGS := -Og -march=native -fno-omit-frame-pointer -gdwarf-5 -glldb $(SANITIZER)
RELEASEFLAGS := -O3 -march=native -DNDEBUG
OPTIMIZATION := $(RELEASEFLAGS)

DEPS_OBJ := base.o UUID.o CSV.o IO.o Algorithms.o LibrarySystem.o

all: main.out test_uuid.out test_quick_sort.out test_binary_search.out

cpp_modules: base.pcm UUID.pcm CSV.pcm IO.pcm Algorithms.pcm LibrarySystem.pcm

# main
main.out: main.o $(DEPS_OBJ)
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) $(LDFLAGS) $^ -o $@

main.o: main.cxx cpp_modules
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -c $< -o $@

# test_uuid
test_uuid.out: test_uuid.o $(DEPS_OBJ)
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) $(LDFLAGS) -DTEST_UUID $^ -o $@

test_uuid.o: main.cxx cpp_modules
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -DTEST_UUID -c $< -o $@

# test_quick_sort
test_quick_sort.out: test_quick_sort.o $(DEPS_OBJ)
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) $(LDFLAGS) -DTEST_QUICK_SORT $^ -o $@

test_quick_sort.o: main.cxx cpp_modules
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -DTEST_QUICK_SORT -c $< -o $@

# test_binary_search
test_binary_search.out: test_binary_search.o $(DEPS_OBJ)
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) $(LDFLAGS) -DTEST_BINARY_SEARCH $^ -o $@

test_binary_search.o: main.cxx cpp_modules
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -DTEST_BINARY_SEARCH -c $< -o $@

# precompiled modules
# base
base.pcm: base.cppm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) --precompile $< -o $@

base.o: base.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -c $< -o $@

# UUID
UUID.pcm: UUID.cppm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) --precompile $< -o $@

UUID.o: UUID.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -c $< -o $@

# CSV
CSV.pcm: CSV.cppm base.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) --precompile $< -o $@

CSV.o: CSV.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -c $< -o $@

# IO
IO.pcm: IO.cppm base.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) --precompile $< -o $@

IO.o: IO.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -c $< -o $@

# Algorithms
Algorithms.pcm: Algorithms.cppm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) --precompile $< -o $@

Algorithms.o: Algorithms.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -c $< -o $@

# LibrarySystem
LibrarySystem.pcm: LibrarySystem.cppm base.pcm UUID.pcm CSV.pcm IO.pcm Algorithms.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) --precompile $< -o $@

LibrarySystem.o: LibrarySystem.pcm
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION) -c $< -o $@

clean:
	rm -f *.out *.ll *.pcm
