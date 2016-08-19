# evcollect unix.stats

different plugins that can each used by itself or combined 

## Tables

###unix.kernel
<table>
  <tbody>
    <tr>
      <td>uptime</td>
      <td>Integer</td>
      <td>Seconds of uptime</td>
    </tr>
    <tr>
      <td>load_avg.min1</td>
      <td>Integer</td>
      <td>Load average during the last minute</td>
    </tr>
    <tr>
      <td>load_avg.min5</td>
      <td>Integer</td>
      <td>Load average during the last five minutes</td>
    </tr>
    <tr>
      <td>load_avg.min15</td>
      <td>Integer</td>
      <td>Load average during the last 15 minutes</td>
    </tr>
    <tr>
      <td>arguments</td>
      <td>String</td>
      <td>Kernel arguments</td>
    </tr>
    <tr>
      <td>version</td>
      <td>Integer</td>
      <td>Kernel version</td>
    </tr>
  </tbody>
</table>


###unix.disk_usage
<table>
  <tbody>
    <tr>
      <td>disk.filesystem</td>
      <td>String</td>
      <td>The name of the file system</td>
    </tr>
    <tr>
      <td>disk.mount_point</td>
      <td>String</td>
      <td>The mount point</td>
    </tr>
    <tr>
      <td>disk.total</td>
      <td>Integer</td>
      <td>The total size of the file system in bytes</td>
    </tr>
    <tr>
      <td>disk.available</td>
      <td>Integer</td>
      <td>The total free space in bytes</td>
    </tr>
    <tr>
      <td>disk.used</td>
      <td>Integer</td>
      <td>The total space used in the file system in bytes</td>
    </tr>
    <tr>
      <td>disk.capacity</td>
      <td>Integer</td>
      <td>The percentage of space that is used between 0 and 100</td>
    </tr>
    <tr>
      <td>disk.iused</td>
      <td>Integer</td>
      <td>The number of used inodes</td>
    </tr>
    <tr>
      <td>disk.ifree</td>
      <td>Integer</td>
      <td>The number of free inodes</td>
    </tr>
  </tbody>
</table>


###unix.processes
<table>
  <tbody>
    <tr>
      <td>process.pid</td>
      <td>The process ID<td>
    </tr>
    <tr>
      <td>process.name</td>
      <td>The filename of the executable<td>
    </tr>
    <tr>
      <td>process.state</td>
      <td>The process state<td>
    </tr>
    <tr>
      <td>process.ppid</td>
      <td>The parent PID of the process<td>
    </tr>
    <tr>
      <td>process.pgrp</td>
      <td>The group ID of the process<td>
    </tr>
    <tr>
      <td>process.ppid</td>
      <td>The PID of the parent of the process<td>
    </tr>
    <tr>
      <td>process.utime</td>
      <td>The time, measured in clock ticks, spent the process spent in user mode<td>
    </tr>
    <tr>
      <td>process.stime</td>
      <td>The time, measured in clock ticks, spent the process spent in kernel mode<td>
    </tr>
    <tr>
      <td>process.nice</td>
      <td>The process nice level (-20 - 19)<td>
    </tr>
    <tr>
      <td>process.starttime</td>
      <td>The time the process started after system boot<td>
    </tr>
    <tr>
      <td>process.vsize</td>
      <td>The virtual memory size in bytes<td>
    </tr>
  </tbody>
</table>


