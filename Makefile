.PHONY: all clean

CXX := clang++
CXXFLAGS := -pipe -std=gnu++23 -fsized-deallocation -Wall -Wextra -fprebuilt-module-path=. -flto=thin
LDFLAGS := -fuse-ld=lld -lstdc++exp -luuid
SANITIZER := -fsanitize=address,undefined
DEBUGFLAGS := -Og -fno-omit-frame-pointer -gdwarf-5 -glldb
RELEASEFLAGS := -O3 -march=native -DNDEBUG
OPTIMIZATION := $(DEBUGFLAGS)

DEPS_OBJ := base.o UUID.o CSV.o IO.o Algorithms.o LibrarySystem.o

all: main.out test_uuid.out test_quick_sort.out test_binary_search.out

cpp_modules: base.pcm UUID.pcm CSV.pcm IO.pcm Algorithms.pcm LibrarySystem.pcm

# main
main.out: main.o $(DEPS_OBJ)
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) $(LDFLAGS) $^ -o $@

main.o: main.cxx cpp_modules
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -c $< -o $@

# test_uuid
test_uuid.out: test_uuid.o $(DEPS_OBJ)
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) $(LDFLAGS) -DTEST_UUID $^ -o $@

test_uuid.o: main.cxx cpp_modules
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -DTEST_UUID -c $< -o $@

# test_quick_sort
test_quick_sort.out: test_quick_sort.o $(DEPS_OBJ)
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) $(LDFLAGS) -DTEST_QUICK_SORT $^ -o $@

test_quick_sort.o: main.cxx cpp_modules
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -DTEST_QUICK_SORT -c $< -o $@

# test_binary_search
test_binary_search.out: test_binary_search.o $(DEPS_OBJ)
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) $(LDFLAGS) -DTEST_BINARY_SEARCH $^ -o $@

test_binary_search.o: main.cxx cpp_modules
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -DTEST_BINARY_SEARCH -c $< -o $@

# precompiled modules
# base
base.pcm: base.cppm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) --precompile $< -o $@

base.o: base.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -c $< -o $@

# UUID
UUID.pcm: UUID.cppm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) --precompile $< -o $@

UUID.o: UUID.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -c $< -o $@

# CSV
CSV.pcm: CSV.cppm base.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) --precompile $< -o $@

CSV.o: CSV.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -c $< -o $@

# IO
IO.pcm: IO.cppm base.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) --precompile $< -o $@

IO.o: IO.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -c $< -o $@

# Algorithms
Algorithms.pcm: Algorithms.cppm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) --precompile $< -o $@

Algorithms.o: Algorithms.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -c $< -o $@

# LibrarySystem
LibrarySystem.pcm: LibrarySystem.cppm base.pcm UUID.pcm CSV.pcm IO.pcm Algorithms.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) --precompile $< -o $@

LibrarySystem.o: LibrarySystem.pcm
	$(CXX) $(CXXFLAGS) $(SANITIZER) $(OPTIMIZATION) -c $< -o $@

clean:
	rm -f *.out *.ll *.pcm
