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
`./full-emulation/<00,15,17>-*/run.sh`.

Finally, `./scripts/parse-logs.sh` and `./scripts/plot-figs.sh` may be used to
process the experiment results and output corresponding figures and tables in
`./figures/`.
