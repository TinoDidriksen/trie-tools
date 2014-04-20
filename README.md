# Command Synopsis

## Building a trie
`trie-build [in-file] [out-file]` which takes UTF-8 input in the form of 1 word per line and turns that into a trie, where
* `in-file` can be omitted or `-` to read words from `stdin`
* `out-file` can be omitted or `-` to write trie to `stdout`

## Printing a trie
`trie-print [in-file] [out-file]` which takes input in the form of a trie and outputs UTF-8 with 1 word per line, where
* `in-file` can be omitted or `-` to read trie from `stdin`
* `out-file` can be omitted or `-` to write words to `stdout`

## Browsing a trie
`trie-browse [-d] <trie-file> [in-file] [out-file]` which take UTF-8 input with 1 prefix per line and outputs a JSON array of words matching that prefix, where
* `-d` enables daemon mode, where it will endlessly loop and reopen `in-file` and `out-file` if they are closed
* `trie-file` is required
* `in-file` can be omitted or `-` to read words from `stdin`
* `out-file` can be omitted or `-` to write JSON to `stdout`

An empty input line results in the trie roots being output.

## Spell checking
`trie-spell <trie-file>` which is an Ispell compatible spell checker that takes UTF-8 input from `stdin` and outputs to `stdout`. The results will be within a Levenshtein distance of `max(1,log2(word.length))`.

## Tokenizing
`trie-tokenize [trie-file] [in-file]` which uses a trie to find best-fit tokenizations of a given stream of untokenized text, where
* `trie-file` can be omitted or `-` to read the trie from `stdin`
* `in-file` can be omitted or `-` to read text from `stdin`

`trie-tokenize-apertium` does the same, but outputs in the Apertium stream format.
