# Artifact Evaluation

<!--toc:start-->
- [Artifact Evaluation](#artifact-evaluation)
  - [Preparation](#preparation)
    - [Pre-pull all docker images](#pre-pull-all-docker-images)
    - [Install containerlab for full emulation comparison](#install-containerlab-for-full-emulation-comparison)
  - [Reproduce experiments](#reproduce-experiments)
    - [5.2.1](#521)
    - [5.2.2](#522)
    - [5.3, 5.4](#53-54)
    - [5.5](#55)
    - [Crunch the number and plot figures](#crunch-the-number-and-plot-figures)
  - [Command Summary](#command-summary)
<!--toc:end-->

## Preparation

To start artifact evaluation, please prepare a machine (physical or virtual)
with a new installation of [Ubuntu 24.04](https://releases.ubuntu.com/noble/)
live server. Once it is up and running, please clone the repository and start
from [README.md](/README.md) to set up the environment and make sure the tests
and the basic usage is working.

### Pre-pull all docker images

Please run the following command to pre-pull all docker images to avoid network
I/O overhead that might skew the evaluation results.

```sh
$ ./scripts/prepull.sh   # this pre-pulls all docker images at once
```

### Install containerlab for full emulation comparison

To compare Neo with full emulation (containerlab) experiments, please run the
following to install containerlab.

```sh
$ ./full-emulation/setup.sh   # set up for full emulation experiments
```

## Reproduce experiments

To reproduce the experiments in the paper, we provide `run.sh` scripts in the
corresponding `examples/` and `full-emulation/` directories. Note that because
the full emulation experiments take Neo's input to configure the network, the
Neo counterpart of the experiment must be completed first. For example, to run
`./full-emulation/15-real-networks/run.sh`, `./examples/15-real-networks/run.sh`
must have been completed.

### 5.2.1

For section 5.2.1, the following may be used to obtain the results of Neo.

```sh
$ cd ./examples/00-reverse-path-filtering
$ ./run.sh
```

On the other hand, the following may be used to obtain the results of full
emulation with 100 runs.

```sh
$ cd ./full-emulation/00-reverse-path-filtering
$ ./run.sh

# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: ~12 min
```

### 5.2.2

For section 5.2.2, we look at two examples: `15-real-networks` and
`17-campus-network`. However, since the campus network dataset is under NDA,
unfortunately `17-campus-network` would not work without the dataset. Here we
focus only on the example of `15-real-networks`. Please run the following to
obtain the experiment results of Neo and full emulation.

```sh
$ ./examples/15-real-networks/prepare-datasets.sh

$ ./examples/15-real-networks/run.sh
# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: ~18 min 4.73 sec

$ ./full-emulation/15-real-networks/run.sh
# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: ~1 hr 27 min 8 sec
```

### 5.3, 5.4

For section 5.3 and 5.4, please run the following to get the results of
scalability evaluation and the detailed emulation overhead for Neo.

```sh
$ ./examples/18-fat-tree-datacenter/run.sh
# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: ~24 min
```

### 5.5

For section 5.5, please run the following to evaluate the effect of Neo's
optimizations with the example `03-load-balancing`.

```sh
$ ./examples/03-load-balancing/run.sh
# ---[ Ubuntu 24.04 VM with 16 core vCPU and 32 GiB RAM ]---
# elapsed time: ~11 min 8.02 sec

# This runs the unoptimized version of Neo with `./examples/03-load-balancing.unopt`.
# The current script `./examples/03-load-balancing.unopt/run.sh` times out after
# an hour, one may increase and decrease the timeout value as needed.
$ ./scripts/run-unopt.sh

# Restore any previous temporary changes that disabled optimizations.
$ git restore .
```

### Crunch the number and plot figures

Finally, please run the following to process the experiment results. They should
output the corresponding figures and tables in `./figures/`.

```sh
$ ./scripts/parse-logs.sh
$ ./scripts/plot-figs.sh
$ ls -al ./figures/
```

## Command summary

To summarize, you may want to try the following:

```sh
# Once finished all the setup process in README.md...

$ ./scripts/prepull.sh   # this pre-pulls all docker images at once

# Run neo experiments
$ ./examples/00-reverse-path-filtering/run.sh
$ ./examples/03-load-balancing/run.sh
$ ./examples/15-real-networks/prepare-datasets.sh
$ ./examples/15-real-networks/run.sh
$ ./examples/18-fat-tree-datacenter/run.sh

# Run full emulation experiments
$ ./full-emulation/setup.sh
$ ./full-emulation/00-reverse-path-filtering/run.sh
$ ./full-emulation/15-real-networks/run.sh

# Run unoptimized version of Neo with examples/03
$ ./scripts/run-unopt.sh
$ git restore .

# Crunch the numbers and plot figures
$ ./scripts/parse-logs.sh
$ ./scripts/plot-figs.sh
$ ls -al ./figures/
```
