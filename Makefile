CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g

OBJ_SHARED = posting.o \
			 avl/avl.o \
			 rbtree/rbtree.o \
			 btree/btree.o \
             index/index.o \
			 index/search.o \
			 instruments/lab3/vector/generic.o \
			 instruments/levenshtein/levenshtein.o

.PHONY: all app u_tests test clean server

all: app u_tests server

app: $(OBJ_SHARED) main.o
	$(CC) $(CFLAGS) -o app $(OBJ_SHARED) main.o

# Правила сборки для сетевого сервера
server: $(OBJ_SHARED) server.o
	$(CC) $(CFLAGS) -o server $(OBJ_SHARED) server.o

server.o: server.c
	$(CC) $(CFLAGS) -I. -c server.c -o server.o

test_avl: posting.o \
		  avl/avl.o \
		  avl/tests.o \
		  instruments/lab3/vector/generic.o
	$(CC) $(CFLAGS) -o test_avl \
					   posting.o \
					   avl/avl.o \
					   avl/tests.o \
					   instruments/lab3/vector/generic.o

test_rb: posting.o \
         rbtree/rbtree.o \
		 rbtree/tests.o \
		 instruments/lab3/vector/generic.o
	$(CC) $(CFLAGS) -o test_rb \
					   posting.o \
					   rbtree/rbtree.o \
					   rbtree/tests.o \
					   instruments/lab3/vector/generic.o

test_btree: posting.o \
			btree/btree.o \
			btree/tests.o \
			instruments/lab3/vector/generic.o
	$(CC) $(CFLAGS) -o test_btree \
					   posting.o \
					   btree/btree.o \
					   btree/tests.o \
					   instruments/lab3/vector/generic.o

u_tests: test_avl test_rb test_btree
	./test_avl
	./test_rb
	./test_btree

test: app
	@echo "=== E2E: preprocessing ==="
	mkdir -p data/test
	python3 preprocess.py \
		--input  data/test/Questions.csv \
		--output data/test/docs.jsonl
	@echo "=== E2E: indexing ==="
	./app index --type=avl   --data=data/test/docs.jsonl --index=data/test/idx_avl.txt
	./app index --type=rb    --data=data/test/docs.jsonl --index=data/test/idx_rb.txt
	./app index --type=btree --data=data/test/docs.jsonl --index=data/test/idx_btree.txt

clean:
	rm -f *.o avl/*.o rbtree/*.o btree/*.o index/*.o instruments/lab3/vector/*.o instruments/levenshtein/*.o
	rm -f app server test_avl test_rb test_btree