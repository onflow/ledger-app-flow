# Ledger Flow app

## Setup

Initialize the git submodules in this repository:

```sh
git submodule update --init --recursive
```

- Install Docker CE
  - Instructions can be found here: https://docs.docker.com/install/

- We only officially support Ubuntu. It should be possible to do app development is other OSs, but compatibility is not always ensured.

On Ubuntu, you should install the following packages:

```
sudo apt update && apt-get -y install build-essential git wget cmake \
libssl-dev libgmp-dev autoconf libtool
```

- Install `node > v14.0`. We typically recommend using `n` for node version management. (This is used to run emulation tests)

- Install python 3

- Install other dependencies running:

  - `make deps`

## Building the app

After installing the dependencies above, run this command to build the app:

```sh
make
```

## Running tests

```sh
make test_install
```

Run this command each time you want to test changes to the app:

```sh
make # rebuild the app to test changes
make test
```
