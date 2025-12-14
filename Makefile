CFLAGS=-Wall -Wextra

bpe: main.c
	$(CC) $(CFLAGS) -o $@ $<
