# report

Internally used library to report execution.
`report` is not directly defined as a library, but redirects to
other report libraries, such as `report-none` (doing nothing) or `report-log` (logging to console).
It can be overriden to satisfy any desired behaviour.

### report.incantation (name)

An incantation for a material named **name** will begin.

### report.invocation (name, ritualname)

An invocation upon a material named **name** will begin.
If the invoked ritual is not anonymous, **ritualname** is the given name of the resolved ritual.
If not, **ritualname** is the number of the ritual executed in the incantation.

### report.copy (source, destination)

Reports the beginning of a copy of file(s) from **source** to **destination**.

### report.remove (path)

Reports the beginning of a removal of file(s) at **path**.

### report.preprocess (source, destination, variables)

Reports the beginning of the preprocessing of **source** into **destination** according to **variables**.

### report.failure (message)

Reports a critical failure raised with the message **message**.

