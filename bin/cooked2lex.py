#!/usr/bin/python -O
# -*- coding: utf-8 -*-

#
# Ulrik Sandborg-Petersen, after Ingo Schröder's cooked2lex.pl
#

import sys
import os
import getopt
import math

def usage(cmd):
    sys.stderr.write("""Usage: %s [-c] [-h]

Options:
    -h Display help message and exit.
    -c Add word-counts.

Reads from stdin. Writes output to stdout. Writes informative and
error messages on stderr.
""" % cmd)

    sys.exit(1)

def cmp_second_first(a,b):
    a2 = a[1]
    b2 = b[1]
    if a2 < b2:
        return -1
    elif a2 > b2:
        return 1
    else:
        a1 = a[0]
        b1 = b[0]
        if a1 < b1:
            return 1
        elif a1 > b1:
            return -1
        else:
            return 0


def rev_cmp_second_first(a,b):
    return -1 * cmp_second_first(a,b)
        

def do_it(bDoCounts):
    number_of_words = 0
    lineno = 0
    word_count_dict = {}
    tag_count_dict = {}
    word_tag_count_dict = {}

    for line in sys.stdin:
        lineno += 1
        if lineno % 100 == 0:
            print >> sys.stderr, "%d sentences\r" % lineno
        line = line.strip()
        line_arr = line.split()
        
        for index in xrange(0, len(line_arr), 2):
            number_of_words += 1

            word = line_arr[index]
            tag = line_arr[index+1]

            word_count_dict.setdefault(word, 0)
            tag_count_dict.setdefault(tag, 0)
            word_tag_count_dict.setdefault(word, {}).setdefault(tag, 0)

            word_count_dict[word] += 1
            tag_count_dict[tag] += 1
            word_tag_count_dict[word][tag] += 1

    print >> sys.stderr, "%d sentences\n" % lineno

    tac_token = {}
    tac_type = {}
    maxx = -1
    for word in sorted(word_count_dict):
        sys.stdout.write(word)
        if bDoCounts:
            sys.stdout.write(" %d" % word_count_dict[word])

        x = 0
        word_count = word_count_dict[word]
        
        mylist = []
        for tag in word_tag_count_dict[word]:
            freq = word_tag_count_dict[word][tag]
            mylist.append((tag, freq))
        mylist.sort(rev_cmp_second_first)

        for (tag, freq) in mylist:
            x += 1
            sys.stdout.write(" %s %d" % (tag, freq))

            word_count -= word_tag_count_dict[word][tag]

        if word_count != 0:
            sys.stder.write("ERROR: inconsistency for %s\n" % word)
            sys.exit(1)

        del mylist

        tac_token.setdefault(x, 0)
        tac_type.setdefault(x, 0)

        tac_token[x] += word_count_dict[word]
        tac_type[x] += 1

        if x > maxx:
            maxx = x

        sys.stdout.write("\n")

    sys.stderr.write("%d tags %d types %d tokens\n" % (len(tag_count_dict), len(word_count_dict), number_of_words))

    ambiguity = 0.0
    for index in xrange(1, maxx+1):
        sys.stderr.write("%3d %9d %7.3f%% %9d %7.3f%% \n" % (index, tac_type[index], (100.0*tac_type[index])/len(word_count_dict), tac_token[index], (100.0*tac_token[index])/number_of_words))
        ambiguity += tac_token[index] * index
    ambiguity /= number_of_words

    sys.stderr.write("Mean ambiguity A=%f\n" % ambiguity)

    entropy = 0.0
    for tag in tag_count_dict:
        p = (1.0 * tag_count_dict[tag]) / number_of_words
        entropy -= p * math.log(p, 2)
    
    sys.stderr.write("\nEntropy H(p)=%f\n" % entropy)


def parse_options():
    (options, arguments) = getopt.getopt(sys.argv[1:], "ch")

    cmd = os.path.split(sys.argv[0])[1]

    bDoCounts = False

    if len(arguments) > 0:
        usage(cmd)
    else:
        for (opt, val) in options:
            if opt == "-h":
                usage(cmd)
            elif opt == "-c":
                bDoCounts = True
            else:
                sys.stderr.write("""Unknown option: %s\n\n""" % opt)
                usage()

    return bDoCounts


bDoCounts = parse_options()
do_it(bDoCounts)
