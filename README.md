# PushAuth-cpp
PushAuth authorization application via mobile Push, Code, QR

## About
This source code of PushAuth client application. Client can send Push request to mobile devices, 
push security codes or generate QR-codes for read their mobile devices.

## Project information
This project was created by CMake. And for build use cmake.

## Used libraries
Thanks to developers of best libraries:

1. LibCurl
2. OpenSSL
3. JSON (https://github.com/nlohmann/json)
4. ProgramOptions.hxx (https://github.com/Fytch/ProgramOptions.hxx)

## How to use?

This console application based at https://pushauth.io/api/index.html API and support all methods.

### Configuration file
By default application find `pushauth.conf` file in `../conf/` directory. But you can use your path to `pushauth.conf`.

Example `pushauth.conf` file:

```
; PushAuth configuration file

[api]                                             ; API configuration
version=1                                         ; v.1
url=https://api.pushauth.io                       ; API endpoint

[keys]
public_key  = tmukeggpDg51dCibDBVdOmWPKrsWgwZY    ; Public key
private_key = iCD90UOsz0VxX5JOFRm1LdiPTrqOOsza    ; Private key

```


### Available options

```shell
$ ./pushauth -?
Usage:
  pushauth [options]
Available options:
  -s, --status    get remote answer status by request hash
  -?, --help      print this help screen
  -r, --response  waiting for client answer. Ignoring for --mode=code
  -v, --hash      show request hash
  -t, --to        email address of client
  -i, --config    path to config file
  -c, --code      code value only for --mode=code
  -m, --mode      type of request: push, code or qr
```

### Push request

#### Send push request to client and wait response from client device
```
$ ./pushauth -i=/home/client/pushauth/conf/pushauth.conf \
                -m=push -t=client@example.com -v
```

Output:
```
Request hash: "z1WZY9qKFJTgvYgOGuO6gzGKsRuB3Odn"
Answer: false
```

#### Send push request to client and without waiting response from client device
```
$ ./pushauth -i=/home/client/pushauth/conf/pushauth.conf \
                -m=push -t=client@example.com -v -r
```

Output:
```
Request hash: "z1WZY9qKFJTgvYgOGuO6gzGKsRuB3Odn"
```

### Push security code

#### Send push code to client device
```
$ ./pushauth -i=/home/client/pushauth/conf/pushauth.conf \
                -m=code -t=client@example.com \
                -c=123456
```

Output:
```
Request hash: "z1WZY9qKFJTgvYgOGuO6gzGKsRuB3Odn"
```

### QR-code authorization

#### Show URL to QR-image without unique request hash
```
$ ./pushauth -i=/home/client/pushauth/conf/pushauth.conf \
                -m=qr
```

Output:
```
QR-image URL: "https://api.pushauth.io/qr/show/image/eyJoYXNoIjoicUJOTkNLZDZiR2pXVENST09NSzk4NDNKaFJRMXgwM00iLCJzaXplIjoiMTI4IiwiY29sb3IiOiI0MCwwLDQwIiwiYmFja2dyb3VuZENvbG9yIjoiMjU1LDI1NSwyNTUiLCJtYXJnaW4iOiIxIn0="
```


#### Show URL to QR-image with unique request hash
```
$ ./pushauth -i=/home/client/pushauth/conf/pushauth.conf \
                -m=qr -v
```

Output:
```
Request hash: "iSgYZNq1CBZWeITg2DGavHughAPuaOnr"
QR-image URL: "https://api.pushauth.io/qr/show/image/eyJoYXNoIjoicUJOTkNLZDZiR2pXVENST09NSzk4NDNKaFJRMXgwM00iLCJzaXplIjoiMTI4IiwiY29sb3IiOiI0MCwwLDQwIiwiYmFja2dyb3VuZENvbG9yIjoiMjU1LDI1NSwyNTUiLCJtYXJnaW4iOiIxIn0="
```

### Show authorization status
This command return if remote client answer:

Output   | Param | Description
---------| --- | -----------
| true   | yes | client answered yes
| false  | no  | client answered no
| timeout| null| client not answer


```
$ ./pushauth -i=/home/client/pushauth/conf/pushauth.conf \
                -s=iSgYZNq1CBZWeITg2DGavHughAPuaOnr
```

Output:
```
Answer: true
```

### Documentation

Please see: https://dashboard.pushauth.io/api/index.html

### Support

Please see: http://dashboard.pushauth.io/support/request/create
