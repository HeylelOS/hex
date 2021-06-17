# report

Internally used library to report execution.
`report` is not directly defined as a library, but redirects to
others report libraries, such as `report-none` (doing nothing) or `report-log` (logging the informations).
It can be overriden to satisfy any behaviour.

### report.incantation (name)

An incantation for a material named **name** will begin.

### report.invocation (name, ritualname)

An invocation upon a material named **name** will begin.
If the invoked ritual is not anonymous, **ritualname** is the given name of the resolved ritual.
If not, **ritualname** is the number of the ritual executed in the incantation.

### report.failure (message)

Reports a critical failure raised with the message **message**.

