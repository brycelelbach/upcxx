EXE = mdo_upcxx

.PHONY: all clean 

all: clean
	make $(EXE)

clean:
	-rm -f *.o $(EXE)

UPCXX_INSTALL_DIR ?= $(HOME)/upcxx_install
include $(UPCXX_INSTALL_DIR)/include/upcxx.mak

VPATH=$(UPCXX_INSTALL_DIR)/include

CXX=mpicxx 

$(EXE).o: $(EXE).cpp 
	$(CXX) $(UPCXX_CXXFLAGS) -o $@ -c $<

$(EXE): $(EXE).o 
	$(CXX) $(LDFLAGS) $(UPCXX_LDFLAGS) -o $@ $^ $(UPCXX_LDLIBS) 

