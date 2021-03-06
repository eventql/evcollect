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

---

#### Table of Contents:
- [Example](#example)
- [Building](#building)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Configuration](#configuration)
- [Plugins](#plugins)
- [Contributing](#contributing)

## Example

An example says more than a thousand words, so here is an illustrative evcollectd
config:

    # submit events to kafka
    output kafka1 plugin kafka
        host 127.0.0.1
        port 1234
        route * %E              # submit all events into topic=<event name>

    # submit events to eventql
    output eventql1 plugin evenql
        host 127.0.0.1
        port 9175
        route * mydb/%E         # store all events into db=mydb and table=<event name>

    # event containing system load statistics. emitted every 30s
    event cluster.system_stats interval 30s
       source plugin linux.systats

    # event containing custom application statistics. emitted every 30s
    event cluster.app_stats 30s
       source shell /usr/local/bin/app_stats.sh

    # submit http access log -- events are emitted as lines are written to the file
    event logs.access_log stream
      source logfile /var/log/nginx/access.log


Once we start evcollectd with the above config, this is what will happen:

  - For each line in /var/log/nginx/access.log, evcollectd will emit a "logs.access_log" event
  - Every 30s, evcollectd will emit a "cluster.system_stats" event containing system load statistics
  - Every 30s, evcollectd will call the "app_stats.sh" shell script and emit the returned JSON as "cluster.app_stats"
  - Every emitted event will be sent to kafka and eventql

Here is what the "logs.access_log" event could look like:

    {
      ...
    }

And here is what the "cluster.system_stats" event could look like:

    {
      ...
    }

## Building

Before we can start we need to install some build dependencies. Currently
you need a modern c++ compiler, libz, autotools

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


## Getting Started

Once you have installed evcollect, create this minimal config file in `/etc/evcollect.conf`
to get started:

    event sysstat interval 1s
      source plugin unix.system_stats

Now you can start the evcollect daemon:

    $ evcollectd --config /etc/evcollect.conf

Note that we didn't pass the `--daemonize` flag so evcollect is running in the
foregrund. Open a new terminal and type this command to see all events as they
are emitted:

    $ evcollectctl monitor

## Usage

The evcollect distribution contains two binaries: `evcollectd` (the main
daemon process) and `evcollectctl` (a command line util.)


#### evcollectd

The main daemon process.

    Usage: $ evcollectd [OPTIONS]

       -s, --spool_dir <dir>     Where to store temporary files
       -c, --config <file>       Load config from file
       -p, --plugin <path>       Load a plugin (.so)
       -P, --plugin_path <dir>   Set the plugin search path
       --daemonize               Daemonize the server
       --pidfile <file>          Write a PID file
       --loglevel <level>        Minimum log level (default: INFO)
       --[no]log_to_syslog       Do[n't] log to syslog
       --[no]log_to_stderr       Do[n't] log to stderr
       -?, --help                Display this help text and exit
       -v, --version             Display the version of this binary and exit

    Examples:
       $ evcollectd --log_to_syslog --daemonize --config /etc/evcollect.conf


#### evcollectctl list

Connects to the main daemon process and lists all configured events and targets.

#### evcollectctl monitor

Connects to the main daemon process and dumps all events to the console as they
are emitted.

## Configuration

## Plugins

### Source Plugins

<table>
  <tr>
    <th>Plugin Name</th>
    <th>Defined Sensors</th>
  <tr>

  <tr>
    <td valign="top">shell (<a href="">Example</a>)</td>
    <td>
      <ul>
        <li>
          User Defined
        </li>
      </ul>
    </td>
  </tr>

  <tr>
    <td valign="top">logfile (<a href="">Example</a>)</td>
    <td>
      <ul>
        <li>
          User Defined
        </li>
      </ul>
    </td>
  </tr>

  <tr>
    <td valign="top">plugin: hostname (<a href="">Example</a>)</td>
    <td>
      <ul>
        <li>
          <b>Event:</b> hostname
          <ul>
            <li>hostname</li>
          </ul>
        </li>
      </ul>
    </td>
  </tr>

  <tr>
    <td valign="top">plugin: unix (<a href="">Example</a>)</td>
    <td>
      <ul>
        <li>
          <b>Event:</b> unix.full_stats
          <ul>
            <li>CPU Usage, CPU Usage, CPU Usage, CPU Usage</li>
          </ul>
        </li>
        <li>
          <b>Event:</b> unix.disk_stats
          <ul>
            <li>CPU Usage, CPU Usage, CPU Usage, CPU Usage</li>
          </ul>
        </li>
      </ul>
    </td>
  </tr>

</table>

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

## License

evcollect is distributed under the AGPL v3 license, see LICENSE file for details.

