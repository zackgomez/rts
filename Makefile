CXX=g++
GLM=lib/glm-0.9.2.7
JSON=lib/jsoncpp
CXXFLAGS=-g -O0 -Wall -I$(GLM) -std=c++0x -I$(JSON)
LDFLAGS=-lSDL -lGL -lGLEW -lglut
OBJDIR=obj
SRCDIR=src

OBJECTS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(SRCDIR)/*.cpp))) $(JSON)/jsoncpp.o

all: obj rts

rts: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<

$(JSON)/jsoncpp.o: $(JSON)/jsoncpp.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $^

clean:
	rm -f rts obj/*
	rm -rf obj/

force_look:
	true

obj:
	mkdir -p obj

.PHONY: clean force_look config
