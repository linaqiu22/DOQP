#!/usr/bin/python3


import sys
from math import *
import random
import numpy


def join(table1, table2):
    out = []
    i, j, prev_j = 0, 0, 0
    while i < len(table1) and j < len(table2):
        j1, d1 = table1[i]
        j2, d2 = table2[j]
        if j1 == j2:
            out.append((d1, d2))
            j += 1
        elif j1 < j2:
            i += 1
            j = prev_j
        else:
            j += 1
            prev_j = j
    return out    


if __name__ == '__main__':
    # n = int(sys.argv[1])
    # n1 = n // 2
    n1 = int(sys.argv[1])
    n2 = int(sys.argv[2])
    n = int(sys.argv[3])
    example_type = sys.argv[4]
    in_file = sys.argv[5]
    in_file2 = sys.argv[6]
    exp_file = sys.argv[7] if len(sys.argv) >= 8 else None
    avg_multiplicity = (n1+n2)/200
    if example_type == '1':
        table1j = [0] * n1
        table2j = [0] * 1 + [1] * (n1 - 1)
    elif example_type == '2':
        table1j = [0] * 1 + [1] * (n1 - 1)
        table2j = [0] * n1
    elif example_type == '3':
        #modified to create one table follows uniform distribution
        #n1 is table size, n2 is dimension, n is domain (default == n1)

        table1j = []
        table2j = []
        table3j = []
        table4j = []
        domain = n
        while len(table1j) < n1:
            key1j = random.randrange(1, domain+1)
            table1j += [key1j]
        while len(table2j) < n1:
            key2j = random.randrange(1, domain+1)
            table2j += [key2j]
        if n2 > 2:
            while len(table3j) < n1:
                key3j = random.randrange(1, domain+1)
                table3j += [key3j]
        if n2 > 3:
            while len(table4j) < n1:
                key4j = random.randrange(1, domain+1)
                table4j += [key4j]
    elif example_type == '4':
        table1j = []
        table2j = []
        out_size = 0
        i = 1
        # avg_multiplicity = int(sqrt(10*n/n1))
        # print(int(sqrt(10*n/n1)))
        # while max(len(table1j), len(table2j), out_size) < n: #n1
        h_max = 0
        w_max = 0
        while out_size < n and len(table1j) < n1 and len(table2j) < n2:
            h = numpy.random.zipf(1.5)
            while h  >= avg_multiplicity:
                h = numpy.random.zipf(1.5)
            w = numpy.random.zipf(1.5)
            while w >= avg_multiplicity:
                w = numpy.random.zipf(1.5)
            
            if h > h_max:
                h_max = h
            if w > w_max:
                w_max = w
            table1j += [i] * h
            table2j += [i] * w
            # print(i, h, w)
            if out_size + h*w <= n:
                out_size += h*w
                i += 1        
    else:
        print("{} is not a valid input class".format(example_type))
        exit()

    if example_type == '4':
        print("h_max", h_max, "w_max", w_max)
        pad1 = max(0, n1 - len(table1j))
        pad2 = max(0, n2 - len(table2j))
        tmp_i = i
        if out_size >= n:
            print("outsize >= n", out_size, "pad1", pad1, "pad2", pad2)
            while pad1 > 0:
                gsize = numpy.random.zipf(2)
                # gsize = random.randrange(1, 11)
                while gsize >= avg_multiplicity or gsize > pad1:
                    gsize = numpy.random.zipf(2)
                table1j += [i] * gsize
                pad1 -= gsize
                i += 1

            while pad2 > 0:
                gsize = numpy.random.zipf(2)
                while gsize >= avg_multiplicity or gsize > pad2:
                    gsize = numpy.random.zipf(2)
                table2j += [i] * gsize
                pad2 -= gsize
                i += 1
        elif out_size < n:
            #TODO the last i should be modified to pad the outsize
            print("outsize < n", n-out_size)
            print(i,h,w)
            org = h * w
            new_cross_product = org + n - out_size
            h_new = sqrt(new_cross_product)
            w_new = h_new
            table1j = table1j[:-h]
            table2j = table2j[:-w]
            table1j += [i-1] * int(h_new)
            table2j += [i-1] * int(w_new)
            if len(table1j) < n1:
                table1j += [max(table1j + table2j)+1] * (n1-len(table1j))
            if len(table2j) < n2:
                table2j += [max(table1j + table2j)+1] * (n1-len(table2j))
    
    if example_type == '3':
        if n2 > 3:
            table1 = sorted(zip(table1j, table2j, table3j, table4j))
        elif n2 > 2:
            table1 = sorted(zip(table1j, table2j, table3j))
        else:
            table1 = sorted(zip(table1j, table2j))
    else:
        # ideally these should be filled with random values as above,
        # but this is problematic if we don't sort at the end
        table1d = table1j[:]
        table2d = table2j[:]
        
        table1 = sorted(zip(table1j, table1d))
        table2 = sorted(zip(table2j, table2d))   
        if exp_file: out = sorted(join(table1, table2))
        
    if example_type != '3':
        with open(in_file, 'w+') as file:
            file.write(str(len(table1)) + " " + str(2) + "\n\n")
            for (j, d) in table1:
                    file.write(str(j) + " " + str(d) + "\n")
        with open(in_file2, "w+") as file2:
            file2.write(str(len(table2)) + " " + str(2) + "\n\n")
            for (j, d) in table2:
                file2.write(str(j) + " " + str(d) + "\n")
        if exp_file:
            with open(exp_file, 'w+') as file:
                for (d1, d2) in out:
                    file.write(str(d1) + " " + str(d2) + "\n")
    else:
        with open(in_file, 'w+') as file:
            file.write(str(len(table1)) + " " + str(n2) + "\n\n")
            if n2 > 3:
                for (j, d, r, t) in table1:
                    file.write(str(j) + " " + str(d) + " " + str(r) + " " + str(t) + "\n")
            elif n2 > 2:
                for (j, d, r) in table1:
                    file.write(str(j) + " " + str(d) + " " + str(r) + "\n")
            else:
                for (j, d) in table1:
                    file.write(str(j) + " " + str(d) + "\n")
            # file.write("\n")
            # for (j, d) in table2:
            #     file.write(str(j) + " " + str(d) + "\n")
