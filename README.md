# bpe

A small prototype of one of the commonly-used tokenization algorithm called [Byte-Pair Encoding](https://en.wikipedia.org/wiki/Byte-pair_encoding) in C.

## Usage

```bash
$ make
$ ./bpe tokenize example.txt example.bin # for tokenizing a txt file
$ ./bpe load example.bin # for loading a bin file created using the above command
```
