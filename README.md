
# General webapp backend

[![Build Status](https://travis-ci.org/aki5/general.svg?branch=master)](https://travis-ci.org/aki5/general)

## Build instructions
```
git clone https://github.com/aki5/general.git
cd general
git submodule update --init --recursive
make test
```

After a while, if the build and the final test is successful
you'll see the following message

```
5000 put+get+deletes in 943318000 nsec, 15901.318537 qps
```

## Run instructions

```
./general -c certs/localhost.pem -l 127.0.0.1:8443
```

then point your browser to https://localhost:8443/
