CFLAGS = -std=c++17 -O3

SRCDIR = src
OUTDIR = build
HEADERS = $(wildcard $(SRCDIR)/*.hpp)
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(OUTDIR)/codingame $(OUTDIR)/interactive $(OUTDIR)/legal_move_test $(OUTDIR)/make_legal_move_test_data $(OUTDIR)/random_match
clean:
	rm -rf $(OUTDIR)/* $(SRCDIR)/*.o

%.o: %.cpp $(HEADERS)
	g++ -c $(CFLAGS) -o $@ $<

$(OUTDIR)/codingame.cpp: src/main_codingame.cpp $(HEADERS)
	mkdir -p $(@D)
	python concat_source.py -o $@ src main_codingame.cpp

$(OUTDIR)/codingame: $(OUTDIR)/codingame.o
	mkdir -p $(@D)
	g++ -o $@ $^ $(CFLAGS)

$(OUTDIR)/interactive: $(SRCDIR)/main_interactive.o
	mkdir -p $(@D)
	g++ -o $@ $^ $(CFLAGS)

$(OUTDIR)/legal_move_test: $(SRCDIR)/main_legal_move_test.o
	mkdir -p $(@D)
	g++ -o $@ $^ $(CFLAGS)

$(OUTDIR)/make_legal_move_test_data: $(SRCDIR)/main_make_legal_move_test_data.o
	mkdir -p $(@D)
	g++ -o $@ $^ $(CFLAGS)

$(OUTDIR)/random_match: $(SRCDIR)/main_random_match.o
	mkdir -p $(@D)
	g++ -o $@ $^ $(CFLAGS)
