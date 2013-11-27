#!/usr/bin/perl
use strict;
use warnings;

print `svn up --ignore-externals`;
my $revision = `svnversion -n`;
$revision =~ s/^([0-9]+).*/$1/g;
$revision += 1;

`/usr/bin/perl -e 's/TRIE_REVISION = [0-9]+;\$/TRIE_REVISION = $revision;/;' -pi include/tdc_trie.hpp`;

print "Set revision to $revision.\n";
