# log

Logging facility, preferred over `print` to display on the console.

### log.print (level[, message...])

Records a new log entry with level **level**, concatenating **message** in one entry.
Level can be one of the following:
- debug
- info
- notice
- warning
- error

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

