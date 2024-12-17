# ULOG messages

This folder contains the ULOG logging message definitions in `.yaml` format.
These messages are parsed by the `tools/ulog_gen.py` script and source code for handling them is generated in `build/ulog/generated`.

Messages are defined in the following format:
```yaml
name: msg1
description: Message 1 description
fields:
  - name: field1
    type: float
  - name: field2
    type: uint8_t
    description: Second field of the message
...
```

For supported types, or to read more about the ULOG format, visit the [official ULOG PX4 wiki page](https://docs.px4.io/main/en/dev_log/ulog_file_format.html).
