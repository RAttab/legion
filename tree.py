#!/usr/bin/python

with open("res/progs.db", mode="r") as db:
    lines = db.readlines()

graph = {}
items = set()

for line in lines:
    if len(line) < 5: continue
    index_in = line.find('<')
    index_out = line.find('>')
    index_end = line.find('.')

    inputs = set()
    if index_in > 0:
        data = line[index_in+1 : index_out if index_out > 0 else index_end]
        inputs = set(data.split(','))

    outputs = set()
    if index_out > 0:
        outputs = set(line[index_out+1 : index_end].split(','))

    for src in inputs:
        if src not in graph:
            graph[src] = set()
        graph[src] = graph[src] | set([int(x, 16) for x in outputs])
    items = items | inputs | outputs


def dot_key(x):
    return '{:0>2X}'.format(x)

def dot_list(l):
    padded = [dot_key(x) for x in l]
    escaped = ['"{}"'.format(x) for x in padded if x in items]
    return " ".join(escaped)

def dot_color(dot, color, values):
    dot.write('subgraph {{ node [color={}]; {} }}\n' .format(color, dot_list(values)))

def dot_label(dot, label, value):
    key = dot_key(value)
    if key not in items: return
    dot.write('subgraph {{ "{}" [label="{}:{}"] }}\n'.format(key, key, label))

with open("tree.dot", mode="w") as dot:
    dot.write("strict digraph {\n")

    dot_color(dot, "blue", range(0x01,0x20))
    dot_color(dot, "green", range(0x20,0xA0))
    dot_color(dot, "red", range(0xA0,0xB0))
    dot_color(dot, "red", range(0xB0,0xFF))

    for elem in range(1, 27):
        dot_label(dot, chr(ord('A') + (elem - 1)), elem)
    dot_label(dot, "frame", 0x21)
    dot_label(dot, "gear", 0x22)
    dot_label(dot, "fuel", 0x23)
    dot_label(dot, "bonding", 0x24)
    dot_label(dot, "circuit", 0x26)
    dot_label(dot, "neural", 0x27)
    dot_label(dot, "servo", 0x40)
    dot_label(dot, "thruster", 0x41)
    dot_label(dot, "propulsion", 0x42)
    dot_label(dot, "plate", 0x43)
    dot_label(dot, "shielding", 0x44)
    dot_label(dot, "hull", 0x4A)
    dot_label(dot, "core", 0x50)
    dot_label(dot, "matrix", 0x51)
    dot_label(dot, "databank", 0x52)
    dot_label(dot, "worker", 0xA0)
    dot_label(dot, "deploy", 0xB0)
    dot_label(dot, "extract.1", 0xB1)
    dot_label(dot, "printer.1", 0xB4)
    dot_label(dot, "assembly.1", 0xB7)
    dot_label(dot, "storage", 0xBA)
    dot_label(dot, "db.1", 0xC0)
    dot_label(dot, "brain.2", 0xC4)
    dot_label(dot, "legion.1", 0xC6)

    for src, dst in graph.items():
        dot.write('"{}" -> {{ {} }}\n'.format(src, dot_list(dst)))

    dot.write("}\n")
