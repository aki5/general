
# General webapp backend

[![Build Status](https://travis-ci.org/aki5/general.svg?branch=master)](https://travis-ci.org/aki5/general)

## build instructions (on OSX and Linux, might work on BSD)

```
git clone https://github.com/aki5/general.git
cd general
git submodule update --init --recursive
make test
```

after a while, if the build and the final test is successful
you'll see the following message

```
Hello simple World
```

## run instructions

```
./general -c certs/localhost.pem -l 127.0.0.1:8443
```

then point your browser to https://localhost:8443/
