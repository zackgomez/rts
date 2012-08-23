CXX=g++
GLM=lib/glm-0.9.2.7
JSON=lib/jsoncpp
STBI=lib/stbi
STBTT=lib/stb_truetype
KISSNET=lib/kissnet
CXXFLAGS=-g -O0 -Wall -I$(GLM) -std=c++0x -I$(JSON) -I$(KISSNET) -I$(STBI) -Wno-reorder -I$(STBTT) -I$(COMMONDIR)
LDFLAGS=-lSDL -lGL -lGLEW -rdynamic
OBJDIR=obj
SRCDIR=src
RTSDIR=$(SRCDIR)/rts
COMMONDIR=$(SRCDIR)/common
MMDIR=$(SRCDIR)/matchmaker

COMMONOBJ = $(patsubst $(COMMONDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(COMMONDIR)/*.cpp))) $(JSON)/jsoncpp.o $(KISSNET)/kissnet.o
RTSOBJ = $(patsubst $(RTSDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(RTSDIR)/*.cpp)))
MMOBJ = $(patsubst $(MMDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(MMDIR)/*.cpp)))

all: obj rts matchmaker

rts: $(RTSOBJ) $(COMMONOBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

matchmaker: $(MMOBJ) $(COMMONOBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(RTSDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<
$(OBJDIR)/%.o: $(COMMONDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<
$(OBJDIR)/%.o: $(MMDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<

$(JSON)/jsoncpp.o: $(JSON)/jsoncpp.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $^

$(KISSNET)/kissnet.o: $(KISSNET)/kissnet.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $^

clean:
	rm -f rts obj/*
	rm -rf obj/

force_look:
	true

obj:
	mkdir -p obj

tags:
	ctags -R src

.PHONY: clean force_look config tags
