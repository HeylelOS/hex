# log

Logging facility, preferred over `print` to display on the console.

### log.level

Explicits the minimum **level** for which `log.print` doesn't discard messages.

### log.print (level[, message...])

Records a new log entry with level **level**, concatenating **message** in one entry.
Level can be one of the following:
- debug
- info
- notice
- warning
- error
If `log.level` is specified and is a valid level, it specified the minimum level
for which the printed messages won't be discarded. By default, and if `log.level`
is not specified, the level is set to `notice`.

### log.debug (message...)

Records **message** as a debug level entry.

### log.info (message...)

Records **message** as an info level entry.

### log.notice (message...)

Records **message** as a notice level entry.

### log.warning (message...)

Records **message** as a warning level entry.

### log.error (message...)

Records **message** as an error level entry.

