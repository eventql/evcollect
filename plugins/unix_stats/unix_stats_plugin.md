# evcollect unix.stats

## Tables

###unix.uptime
<table>
  <tbody>
    <tr>
      <td>days</td>
      <td>Integer</td>
      <td>Days of uptime</td>
    </tr>
    <tr>
      <td>hours</td>
      <td>Integer</td>
      <td>Hours of uptime</td>
    </tr>
    <tr>
      <td>minutes</td>
      <td>Integer</td>
      <td>Minutes of uptime</td>
    </tr>
    <tr>
      <td>seconds</td>
      <td>Integer</td>
      <td>Seconds of uptime</td>
    </tr>
  </tbody>
</table>


###unix.disk_usage
<table>
  <tbody>
    <tr>
      <td>disk_stats.filesystem</td>
      <td>String</td>
      <td>The name of the file system.</td>
    </tr>
    <tr>
      <td>disk_stats.mount_point</td>
      <td>String</td>
      <td>The mount point.</td>
    </tr>
    <tr>
      <td>disk_stats.total</td>
      <td>Integer</td>
      <td>The total size of the file system in bytes.</td>
    </tr>
    <tr>
      <td>disk_stats.used</td>
      <td>Integer</td>
      <td>The total space used in the file system in bytes</td>
    </tr>
    <tr>
      <td>disk_stats.available</td>
      <td>Integer</td>
      <td>The total free space in bytes.</td>
    </tr>
    <tr>
      <td>disk_stats.capacity</td>
      <td>Integer</td>
      <td>The percentage of space that is used between 0 and 100.</td>
    </tr>
    <tr>
      <td>disk_stats.iused</td>
      <td>Integer</td>
      <td>The number of used inodes.</td>
    </tr>
    <tr>
      <td>disk_stats.ifree</td>
      <td>Integer</td>
      <td>The number of free inodes.</td>
    </tr>
  </tbody>
</table>


###unix.load_avg
<table>
  <tbody>
    <tr>
      <td>load_avg</td>
      <td>Repeated record</td>
      <td></td>
    </tr>
    <tr>
      <td>load_avg.min</td>
      <td>Integer</td>
      <td>One of 1, 5 or 15</td>
    </tr>
    <tr>
      <td>load_avg.value</td>
      <td>Double</td>
      <td>The load average</td>
    </tr>
    <tr>
      <td>freeram</td>
      <td>Integer</td>
      <td>The size of available memory.</td>
    </tr>
    <tr>
      <td>freeswap</td>
      <td>Integer</td>
      <td>The size of available swap space.</td>
    </tr>
  </tbody>
</table>
