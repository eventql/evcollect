# evcollect

evcollect is a lightweight daemon that collects measurements/events from
user-defined sensors and submits them to one or more pluggable event stores.

Examples for sensors (sources) include:
  - Built-in system sensors (e.g. CPU Used, Memory Used, etc)
  - User-defined sensors (scripts that produce a JSON output, e.g. memcache.num_connections)
  - Structured Logfiles (eg. nginx access logs)

evcollect supports the following targets:
  - Logfile
  - EventQL
  - Kafka

## Example

An example says more than a thousand words, so here is an illustrative evcollectd
config:

    # submit events to kafka
    output kafka1 plugin kafka
        host 127.0.0.1
        port 1234

    # submit events to eventql
    output eventql1 plugin evenql
        host 127.0.0.1
        port 9175

    # normal event containing system load statistics. submitted every 30s
    event cluster.system_stats interval 30s
       source plugin linux.systats

    event cluster.app_stats 30s
       source shell /usr/local/bin/app_stats.sh

    # http access log. "streaming event" -- events submitted as they are written
    event logs.access_log stream
      source logfile /var/log/nginx/access.log


Once we start evcollectd with the above config, this is what will happen:

  - For each line in /var/log/nginx/access.log, evcollectd will emit a "logs.access_log" event
  - Every 30s, evcollectd will emit a "cluster.system_stats" event containing system load statistics
  - Every 30s, evcollectd will call the "app_stats.sh" shell script and emit the returned JSON event
  - Every emitted event will be sent to kafka and eventql

## Building

Before we can start we need to install some build dependencies. Currently
you need a modern c++ compiler, libz, autotools and python (for spidermonkey/mozbuild)

    # Ubuntu
    $ apt-get install clang++ cmake make automake autoconf zlib1g-dev

    # OSX
    $ brew install automake autoconf

To build evcollect from a distribution tarball:

    $ ./configure
    $ make
    $ sudo make install

To build evcollect from a git checkout:

    $ git clone git@github.com:eventql/evcollect.git
    $ cd evcollect
    $ ./autogen.sh
    $ ./configure
    $ make V=1
    $ src/evql -h


## Configuration

## Usage


## Plugins

<table>
  <tr>
    <th>Plugin Name</th>
    <th>Defined Sensors</th>
  <tr>
  <tr>
    <td valign="top">linux (<a href="">Example</a>)</td>
    <td>
      <ul>
        <li>
          <b>Sensor:</b> linux.sysstat
          <ul>
            <li>CPU Usage</li>
            <li>Load</li>
            <li>CPU Usage</li>
            <li>CPU Usage</li>
          </ul>
        </li>
      </ul>
    </td>
  </tr>
</table>
