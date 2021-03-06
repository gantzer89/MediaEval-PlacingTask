# Makefile for VocabBuildDB

CXXFLAGS = -O2 $(DEBUGFLAGS) -fmessage-length=0 -std=c++0x
LDFLAGS = -L../lib/ -lboost_regex -lboost_iostreams

# Common
CXXFLAGS += -I../Common/include/
LDFLAGS += -lcommon

# KMajority
CXXFLAGS += -I../KMajorityLib/include
LDFLAGS += -lkmajority

# Incremental K-Means
CXXFLAGS += -I../IncrementalKMeansLib/include
LDFLAGS += -lincrementalkmeans

# VocabLib
CXXFLAGS += -I../VocabLib/include
LDFLAGS += -lvocab

# OpenCV (this goes last: beware of the linking order)
CXXFLAGS += `pkg-config opencv --cflags`
LDFLAGS += `pkg-config opencv --libs`

#LDFLAGS += -Wl,-rpath=../../agast_lib/lib
#LDFLAGS += -Wl,-rpath=../../dbrief_lib/lib
#LDFLAGS += -Wl,-rpath=../lib/

SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

BIN = VocabBuildDB

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CXX) -o $(CXXFLAGS) -o $(BIN) $(OBJECTS) $(LDFLAGS)

clean:
	rm -rf $(OBJECTS) $(BIN) *~
