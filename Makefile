CFLAGS = -std=c++17 -Ofast

SRCDIR = src
OUTDIR = build
HEADERS = $(wildcard $(SRCDIR)/*.hpp)
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)
PYTHON_EXTENSION_SUFFIX = $(shell python3-config --extension-suffix)

.PHONY: all clean

all: $(OUTDIR)/codingame.py $(OUTDIR)/interactive $(OUTDIR)/generate_training_data_1 $(OUTDIR)/legal_move_test $(OUTDIR)/make_legal_move_test_data $(OUTDIR)/print_tree $(OUTDIR)/random_match $(OUTDIR)/test_dnn_evaluator othello_train/othello_train_cpp$(PYTHON_EXTENSION_SUFFIX)
clean:
	rm -rf $(OUTDIR)/* $(SRCDIR)/*.o

%.o: %.cpp $(HEADERS)
	g++ -c $(CFLAGS) -o $@ $<

build/model.a: model/savedmodel/saved_model.pb
	mkdir -p $(@D)
	./tvm/build_model.sh "model/savedmodel" "$@"

$(OUTDIR)/codingame.bin: $(SRCDIR)/main_codingame.cpp $(HEADERS) build/model.a
# サーバのglibcより新しい環境でビルドしてしまうと動作しないため、バージョンを指定したdockerイメージ内のg++を利用
	docker run --rm --mount type=bind,source=$(shell pwd),target=/build gcc:12.2.0-bullseye sh -c 'g++ /build/src/main_codingame.cpp /build/build/model.a -o /build/build/codingame.bin -I/build/tvm/include --std=c++17 -Ofast && strip /build/build/codingame.bin'

$(OUTDIR)/codingame.py: $(OUTDIR)/codingame.bin
	mkdir -p $(@D)
	python scripts/pack_executable_to_py.py $< $@

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

$(OUTDIR)/print_tree: $(SRCDIR)/main_print_tree.o
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
