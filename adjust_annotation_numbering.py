import glob
import os
import shutil
import re

import docopt

docstr = """ adjust the annotation numbering

Usage:
    adjust_annotation_numbering.py [options]

Options:
    -h, --help                  Print this message
    --annotationdir=<str>       Ground truth path prefix
    --outpath=<str>             Ground truth path prefix [default: None]
"""

def natural_sort(s, _nsre=re.compile('([0-9]+)')):
    return [int(text) if text.isdigit() else text.lower()
            for text in re.split(_nsre, s)]

def correct_annotations(logdir, outdir, offset_num):
    if outdir is None:
        outdir = logdir
    if not os.path.exists(outdir):
        os.makedirs(outdir)

    annotations = sorted(glob.glob(os.path.join(logdir, '*.txt')), key=natural_sort)
    for annotation in annotations:
        annotation_dir, annotation_fname = os.path.split(annotation)
        #move the file to offset_num less than its current number, and delete the original file
        annotation_f, annotation_ext = os.path.splitext(annotation_fname)
        dst_fpath = str(int(annotation_f)+offset_num).zfill(len(annotation_f))
        dst_fpath = os.path.join(outdir, dst_fpath + annotation_ext)
        assert(not os.path.exists(dst_fpath))
        shutil.move(annotation, dst_fpath)

if __name__ == "__main__":
    args = docopt.docopt(docstr, version='v0.1')
    print(args)

    logdir = args['--annotationdir']
    outdir = args['--outpath']
    correct_annotations(logdir, outdir, 1)
