CXX=g++
GLM=lib/glm-0.9.2.7
JSON=lib/jsoncpp
STBI=lib/stbi
STBTT=lib/stb_truetype
LDFLAGS=-lSDL -lGL -lGLEW -rdynamic
OBJDIR=obj
SRCDIR=src
RTSDIR=$(SRCDIR)/rts
COMMONDIR=$(SRCDIR)/common
MMDIR=$(SRCDIR)/matchmaker
TESTDIR=$(SRCDIR)/tests
GTESTDIR=lib/gtest-1.6.0
GTESTLIB=lib/gtest-1.6.0/libgtest.a
CXXFLAGS=-g -O0 -Wall -I$(GLM) -std=c++0x -I$(JSON) -I$(STBI) -Wno-reorder -I$(STBTT) -I$(SRCDIR) -I$(GTESTDIR)/include
#CXXFLAGS+=-DSECTION_RECORDING

TESTOBJ = $(patsubst $(TESTDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(TESTDIR)/*.cpp)))
COMMONOBJ = $(patsubst $(COMMONDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(COMMONDIR)/*.cpp))) $(JSON)/jsoncpp.o
RTSOBJ = $(patsubst $(RTSDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(RTSDIR)/*.cpp)))
MMOBJ = $(patsubst $(MMDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(MMDIR)/*.cpp)))

all: obj rts matchmaker tests

rts: $(RTSOBJ) $(COMMONOBJ) local.json
	$(CXX) $(CXXFLAGS) -o $@ $(RTSOBJ) $(COMMONOBJ) $(LDFLAGS)

matchmaker: $(MMOBJ) $(COMMONOBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tests: $(TESTOBJ) $(GTESTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $(TESTOBJ) $(COMMONOBJ) $(GTESTLIB) -lpthread

$(OBJDIR)/%.o: $(RTSDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<
$(OBJDIR)/%.o: $(COMMONDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<
$(OBJDIR)/%.o: $(MMDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<
$(OBJDIR)/%.o: $(TESTDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<

$(JSON)/jsoncpp.o: $(JSON)/jsoncpp.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $^

$(GTESTLIB): $(GTESTDIR)/src/gtest-all.cc
	$(CXX) -I$(GTESTDIR)/include -I$(GTESTDIR) -c $(GTESTDIR)/src/gtest-all.cc -o $(GTESTDIR)/src/gtest-all.o
	$(CXX) -I$(GTESTDIR)/include -I$(GTESTDIR) -c $(GTESTDIR)/src/gtest_main.cc -o $(GTESTDIR)/src/gtest_main.o
	ar -rv $(GTESTDIR)/libgtest.a $(GTESTDIR)/src/gtest-all.o $(GTESTDIR)/src/gtest_main.o


local.json: local.json.default
	cp local.json.default local.json

clean:
	rm -f rts obj/* $(GTESTLIB)
	rm -rf obj/

force_look:
	true

obj:
	mkdir -p obj

tags:
	ctags -R src

.PHONY: clean force_look config tags
