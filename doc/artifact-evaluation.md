# Artifact Evaluation

<!--toc:start-->
- [Artifact Evaluation](#artifact-evaluation)
<!--toc:end-->

To start artifact evaluation, please prepare a machine (physical or virtual)
with a new installation of [Ubuntu 24.04](https://releases.ubuntu.com/noble/)
live server.

Then, please proceed to [README.md](README.md) to get started, set up the
environment, and test run some examples.

To run the Neo experiments, please run `./examples/<15,17,18,03>-*/run.sh`
scripts.

To run the full emulation (containerlab) experiments, please first run
`./full-emulation/setup.sh` to install containerlab, and then run
`./full-emulation/<00,15,17>-*/run.sh`. Note that because the emulation
experiments take Neo's input to configure the network, the Neo counterpart of
the experiment must be completed first. For example, to run
`./full-emulation/15-real-networks/run.sh`, `./examples/15-real-networks/run.sh`
must have been completed.

Also note that since the campus network dataset is under NDA, unfortunately
`17-campus-network` may not work without the data outside the repository.

Finally, `./scripts/parse-logs.sh` and `./scripts/plot-figs.sh` may be used to
process the experiment results and output corresponding figures and tables in
`./figures/`.

To summarize, you may want to try the following:

```bash
# Once finished all the setup process in README.md...

$ ./scripts/prepull.sh   # this pre-pulls all docker images at once

$ ./examples/00-reverse-path-filtering/run.sh
$ ./examples/03-load-balancing/run.sh
$ ./examples/15-real-networks/run.sh
$ ./examples/18-fat-tree-datacenter/run.sh

$ ./full-emulation/setup.sh
$ ./full-emulation/00-reverse-path-filtering/run.sh
$ ./full-emulation/15-real-networks/run.sh

$ ./scripts/run-unopt.sh  # run unoptimized version of Neo with examples/03
$ git restore .
$ ./scripts/parse-logs.sh
$ ./scripts/plot-figs.sh

$ ls -al ./figures/
```
