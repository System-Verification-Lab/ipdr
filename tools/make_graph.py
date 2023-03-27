#!/usr/bin/env python3

from collections import defaultdict
import sys
import os
import re as regex
import argparse
import pprint
import string
from typing import KeysView, List, Mapping

TYPES = ["pebbling", "z3pdr", "bmc"]
tools_folder = os.path.dirname(os.path.realpath(__file__))
with open(tools_folder + "/model_order.txt", 'r') as model_file:
    MODELS = [m.rstrip() for m in model_file.readlines()]

parser = argparse.ArgumentParser(
    description="Parse the contents of an output folder and turn them into a pgfplot bar graph."
)

parser.add_argument("--debug", action="store_true")
parser.add_argument(
    "--dir", type=str, required=True,
    help="the \"output\" folder in which to search for the experiment results."
)
parser.add_argument(
    "--format", type=str, required=True,
    help="the format for the folder and file name of the run: \"{model}-{run-format}\"."
)
parser.add_argument(
    "--types", type=str, nargs="+", choices=TYPES, required=True,
    help=f"the types of algorithm to search results for in the \"output\" folder\". T = {TYPES}",
    metavar="T"
)
modelgroup = parser.add_mutually_exclusive_group(required=True)
modelgroup.add_argument(
    "--allmodels", action="store_true",
    help=f"create a graph for all models {MODELS}"
)
modelgroup.add_argument(
    "--models", nargs="+", type=str, choices=MODELS,
    help=f"a list of model names (from model_order.txt) for which to gather results. M = {MODELS}",
    metavar="M"
)

args = parser.parse_args()
# sort args.models according order in model_order
if args.allmodels:
    args.models = MODELS
else:
    args.models = sorted(args.models, key=MODELS.index)


def main():
    print(f"folder:\t\t\t {args.dir}")
    print(f"content:\t\t {os.listdir(args.dir)}")
    print(f"available models:\t {MODELS}")
    print(f"selected models:\t {args.models}")
    print(f"selected algorithm:\t {args.types}")
    print()

    model_data = {}
    control = False
    for run in args.types:
        for model in args.models:
            if args.debug:
                print(f"model: {model}")

            model_folder = f"{model}-{args.format}"

            if run == "z3pdr":
                control = True

            if control:
                model_folder = add_C_tag(model_folder)

            file_content = get_data(run, model, model_folder)
            table_contents = split_tables(file_content)

            if args.debug:
                print(f"table (size {len(table_contents)})")
                pprint.pprint(table_contents)
                print()

            table = [split_rows(t) for t in table_contents]

            if args.debug:
                print("rows")
                pprint.pprint(table)
                print()

            if control:
                data = parse_rows_single(run, table)
            else:
                data = parse_rows_combined(run, table)

            if args.debug:
                print("data")
                pprint.pprint(data)
                print()

            model_data[model] = data

        plot = bar_data(model_data)

        if args.debug:
            print("plot")
        print(plot)
    return 0


# READ LATEX TABLE
test = r'''\begin{tabular}
{rr}
 & ipdr \\
avg time & 7.010 s \\
std dev time & 0.217 s \\
max inv constraint & 8 \\
min inv level & 15 \\
min strat marked & 9 \\
min strat length & 21 \\
\end{tabular}
\begin{tabular}
{rrrr}
 & ipdr & control & improvement \\
avg time & 7.010 s & 28.395 s & 75.31 \% \\
std dev time & 0.217 s & 2.289 s &  \\
max inv constraint & 8 & 8 &  \\
min inv level & 15 & 8 & 0.00 \% \\
min strat marked & 9 & 9 &  \\
min strat length & 21 & 22 & 4.55 \% \\
\end{tabular}'''


def split_tables(tex: str) -> List[str]:
    pattern = r"\\begin\{tabular\}(?:\n)*\{(?:.)*?\}((?:.|\n)*?)\\end\{tabular\}"
    return [x.replace('\n', '') for x in regex.findall(pattern, tex)]


def split_rows(table: str) -> List[str]:
    rv = [x for x in table.split("\\\\") if x != '']
    return rv


Data_t = Mapping[str, Mapping[str, str]]


def parse_rows_single(type: str, table: List[List[str]]) -> Data_t | None:
    if len(table) == 0:
        return None

    rows = table[0]
    rv = {type: {}}

    for row in rows[1:]:
        cells = row.split('&')
        assert len(cells) == 2

        name = cells[0].strip()
        rv[type][name] = cells[1].strip()

    return rv


def parse_rows_combined(type: str, table: List[List[str]]) -> Data_t | None:
    if len(table) == 0:
        return None

    rows = table[1]
    rv = {type: {}, "control": {}}

    for row in rows[1:]:
        cells = row.split('&')
        assert len(cells) == 4

        name = cells[0].strip()
        rv[type][name] = cells[1].strip()
        rv["control"][name] = cells[2].strip()

    return rv

# write plot


def tabbed(s: str) -> str:
    assert s is not None
    return regex.sub("^", "    ", s, flags=regex.MULTILINE)


def escape(s: str) -> str:
    assert s is not None
    return regex.sub("\\^", "\\^{}", regex.sub("_", "\\_", s))


def add_C_tag(s: str):
    assert s is not None
    return regex.sub("(_[0-9]+)$", r"_C\1", s)


def get_time_val(s: str) -> str:
    assert s is not None
    result = regex.search("([0-9]+\\.[0-9]+) s", s)
    if not result == None:
        return result.group(1)
    return "???"


def name_list(names: KeysView[str]) -> str:
    rv = ""
    for name in names:
        rv += f"    \\texttt{{{name}}},\n"

    return rv


def add_plot(version: str, data: Mapping[str, Data_t]):
    assert version == "ipdr" or version == "control"

    str = ("\\addplot+[\n"
           "    error bars/.cd,\n"
           "    y dir=both,\n"
           "    y explicit\n"
           "] coordinates {\n"
           )

    for name in data:
        time = get_time_val(data[name][version]["avg time"])
        dev = get_time_val(data[name][version]["std dev time"])
        str += f"\t(\\texttt{{{name}}}, {time}) +- (0, {dev})\n"

    str += "};"

    return str


def write_plot(data: Mapping[str, Data_t]) -> str:
    # escape names of data
    data = {escape(key): data[key] for key in data}

    str = "\\begin{figure}\n"
    str += "\\centering\n"
    str += "\\begin{vsplot}{\\cipdr, naive \\cipdr}{\n"
    str += name_list(data.keys())
    str += "}\n"

    str += tabbed(add_plot("ipdr", data)) + "\n"
    str += "\n"
    str += tabbed(add_plot("control", data)) + "\n"

    str += "\\end{vsplot}\n"
    str += "\\end{figure}"

    return str


def bar_data(data: Mapping[str, Data_t]) -> str:
    data = {escape(key): data[key] for key in data}
    output = defaultdict(lambda: defaultdict(tuple))

    str = "PGFPLOT DATA\n"
    for model in data:
        if data[model] is None:
            continue
        for type in data[model]:
            time = get_time_val(data[model][type]["avg time"])
            dev = get_time_val(data[model][type]["std dev time"])
            output[type][model] = (time, dev)

    for type in output:
        str += type + "\n"
        for i, model in zip(string.ascii_lowercase, data):
            if model in output[type]:
                time, dev = output[type][model]
                str += f"({i}, {time}) +- (0, {dev})\t% {model}\n"
            else:
                str += "???"
        str += "\n"

    return str

# main matter


def get_data(sub_dir: str, model: str, model_dir: str) -> str:
    file_path = os.path.join(
        args.dir, sub_dir, model, model_dir, f"{model_dir}.tex")
    if args.debug:
        print(file_path)
    assert os.path.isfile(file_path)
    with open(file_path, 'r') as file:
        raw = file.read()
    return raw


if __name__ == "__main__":
    main()