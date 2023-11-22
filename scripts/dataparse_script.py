import os
import sys
import argparse
sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))
from dataparse import *

def parse_dataset(directory, prefix, dtype):

    if dtype=="snap":
        for filename in os.scandir(directory):
            filepath=os.path.realpath(filename)
            parse_snap(filepath, prefix)

    elif dtype=="rf":
        for filename in os.scandir(directory):
            filepath=os.path.realpath(filename)
            parse_rfuel(filepath, prefix)
    else:
        print("invalid dataset type")



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d',
                        '--directory',
                        type=str,
                        help='dataset directory',
                        required=True)
    parser.add_argument('-p',
                        '--prefix',
                        type=str,
                        help='prefix for output files',
                        required=True)
    parser.add_argument('-t',
                        '--dtype',
                        type = str,
                        help = 'dataset type, snap | rf',
                        required=True)
    args = parser.parse_args()

    parse_dataset(args.directory, args.prefix, args.dtype)

if __name__ == '__main__':
    main()
