# producerConsumerPipeline
Multi-threaded program reads lines of characters from standard input and writes them as 80 character long lines to standard output with specific changes:

1. Every line separator in the input will be replaced by a space
2. Every adjacent pair of plus signs, i.e., "++", is replaced by a "^".
