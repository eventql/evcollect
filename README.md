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
