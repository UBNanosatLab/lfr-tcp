# LFR-TCP

`lfr-tcp` is a bridge between an LFR compatible simulated radio interface and a KISS TNC interface. `lfr-tcp` acts as a server, accepting one client connection at a time on the `uart_port`. Over this port, the client can send LFR commands that will be proccessed by `lfr-tcp`. `lfr-tcp` also acts as a KISS client. It connects to the server specified by `ipaddr` and `port` and sends any packets as KISS commands to that server. It also accepts KISS commands from that server which then are passed as received packets over an LFR interface on `uart_port`.

`com_radio.py` provides an example LFR client, exposing the `radio_util` interface found elsewhere in UBNL code.

## Compiling:

There are no external dependencies.
Simply:

```bash
make
```

The sole build product is `lfr_tcp`.

## Usage:

```
lfr-tcp ipaddr port uart_port
```


## Example
In three separate terminals:

```bash
nc -l 52001 | xxd
```

```bash
./lfr-tcp 127.0.0.1 52001 2600
```

```bash
com_radio.py 127.0.0.1 2600 tx 10
```
