CFLAGS = -std=c++17 -O3

SRCDIR = src
OUTDIR = build
HEADERS = $(wildcard $(SRCDIR)/*.hpp)
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)
PYTHON_EXTENSION_SUFFIX = $(shell python3-config --extension-suffix)

.PHONY: all clean

all: $(OUTDIR)/codingame $(OUTDIR)/interactive $(OUTDIR)/generate_training_data_1 $(OUTDIR)/legal_move_test $(OUTDIR)/make_legal_move_test_data $(OUTDIR)/random_match $(OUTDIR)/test_dnn_evaluator othello_train/othello_train_cpp$(PYTHON_EXTENSION_SUFFIX)
clean:
	rm -rf $(OUTDIR)/* $(SRCDIR)/*.o

%.o: %.cpp $(HEADERS)
	g++ -c $(CFLAGS) -o $@ $<

$(OUTDIR)/codingame.cpp: src/main_codingame.cpp $(HEADERS)
	mkdir -p $(@D)
	python concat_source.py -o $@ src main_codingame.cpp --exclude dnn_evaluator_socket.hpp search_alpha_beta_constant_depth.hpp search_alpha_beta_iterative.hpp search_policy.hpp  search_random.hpp search_mcts_train.hpp

$(OUTDIR)/codingame: $(OUTDIR)/codingame.o
	mkdir -p $(@D)
	g++ -o $@ $^ $(CFLAGS)

$(OUTDIR)/interactive: $(SRCDIR)/main_interactive.o
	mkdir -p $(@D)
	g++ -o $@ $^ $(CFLAGS)

$(OUTDIR)/generate_training_data_1: $(SRCDIR)/main_generate_training_data_1.o
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

$(OUTDIR)/test_dnn_evaluator: $(SRCDIR)/main_test_dnn_evaluator.o
	mkdir -p $(@D)
	g++ -o $@ $^ $(CFLAGS)

othello_train/othello_train_cpp$(PYTHON_EXTENSION_SUFFIX): src/lib_pybind11.cpp $(HEADERS)
	g++ -o $@ $(CFLAGS) -shared -fPIC -Iextern/pybind11/include $(shell python3-config --includes) $<
