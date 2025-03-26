CXX		 := g++
CXXFLAGS := -std=c++20
LDLIBS	 := -lssh

SSH_SOURCE := SSH.cpp
SSH_OBJECT := SSH.o
SSH_BIN	   := SSH

MAIN := main.cpp

$(SSH_BIN): $(MAIN) $(SSH_OBJECT)
	$(CXX) -o $@ $^ $(LDLIBS)

$(SSH_OBJECT): $(SSH_SOURCE)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(SSH_BIN) $(SSH_OBJECT)
